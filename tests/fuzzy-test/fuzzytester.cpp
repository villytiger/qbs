/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of the Qt Build Suite.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/
#include "fuzzytester.h"

#include <QDir>
#include <QElapsedTimer>
#include <QProcess>

#include <algorithm>
#include <cstdlib>
#include <ctime>

FuzzyTester::FuzzyTester()
{
}

void FuzzyTester::runTest(const QString &profile, const QString &startCommit,
                          int maxDurationInMinutes, int jobCount)
{
    m_profile = profile;
    m_jobCount = jobCount;

    runGit(QStringList() << "rev-parse" << "HEAD", &m_headCommit);
    qDebug("HEAD is %s", qPrintable(m_headCommit));

    qDebug("Trying to find a buildable commit to start with...");
    const QString workingStartCommit = findWorkingStartCommit(startCommit);
    qDebug("Found buildable start commit %s.", qPrintable(workingStartCommit));
    QStringList allCommits = findAllCommits(workingStartCommit);
    qDebug("The test set comprises all %d commits between the start commit and HEAD.",
           allCommits.count());

    // Shuffle the initial sequence. Otherwise all invocations of the tool with the same start
    // commit would try the same sequence of commits.
    std::srand(std::time(0));
    std::random_shuffle(allCommits.begin(), allCommits.end());

    quint64 run = 0;
    QStringList buildSequence(workingStartCommit);
    QElapsedTimer timer;
    const qint64 maxDurationInMillis = maxDurationInMinutes * 60 * 1000;
    if (maxDurationInMillis != 0)
        timer.start();

    bool timerHasExpired = false;
    while (std::next_permutation(allCommits.begin(), allCommits.end()) && !timerHasExpired) {
        qDebug("Testing permutation %llu...", ++run);
        foreach (const QString &currentCommit, allCommits) {
           if (timer.isValid() && timer.hasExpired(maxDurationInMillis)) {
               timerHasExpired = true;
               break;
           }

            buildSequence << currentCommit;
            checkoutCommit(currentCommit);
            qDebug("Testing incremental build #%d (%s)", buildSequence.count() - 1,
                   qPrintable(currentCommit));

            // Doing "resolve" and "build" separately introduces additional possibilities
            // for errors, as information from change tracking has to be serialized correctly.
            QString qbsError;
            bool success = runQbs(defaultBuildDir(), QLatin1String("resolve"), &qbsError);
            if (success)
                success = runQbs(defaultBuildDir(), QLatin1String("build"), &qbsError);
            if (success) {
                if (!doCleanBuild(&qbsError)) {
                    const QString message = QString::fromLocal8Bit("An incremental build succeeded "
                            "with a commit for which a clean build failed.\n"
                            "The qbs error message for the clean build was: '%1'").arg(qbsError);
                    throwIncrementalBuildError(message, buildSequence);
                }
            } else {
                qDebug("Incremental build failed. Checking whether clean build works...");
                if (doCleanBuild()) {
                    const QString message = QString::fromLocal8Bit("An incremental build failed "
                            "with a commit for which a clean build succeeded.\n"
                            "The qbs error message for the incremental build was: '%1'")
                            .arg(qbsError);
                    throwIncrementalBuildError(message, buildSequence);
                } else {
                    qDebug("Clean build also fails. Continuing.");
                }
            }
        }
    }

    if (timerHasExpired)
        qDebug("Maximum duration reached.");
    else
        qDebug("All possible permutations were tried.");
}

void FuzzyTester::checkoutCommit(const QString &commit)
{
    runGit(QStringList() << "checkout" << commit);
    runGit(QStringList() << "submodule" << "update" << "--init");
}

QStringList FuzzyTester::findAllCommits(const QString &startCommit)
{
    QString allCommitsString;
    runGit(QStringList() << "log" << (startCommit + "~1.." + m_headCommit) << "--format=format:%h",
           &allCommitsString);
    return allCommitsString.simplified().split(QLatin1Char(' '));
}

QString FuzzyTester::findWorkingStartCommit(const QString &startCommit)
{
    const QStringList allCommits = findAllCommits(startCommit);
    QString qbsError;
    for (int i = allCommits.count() - 1; i >= 0; --i) {
        QString currentCommit = allCommits.at(i);
        checkoutCommit(currentCommit);
        removeDir(defaultBuildDir());
        if (runQbs(defaultBuildDir(), QLatin1String("build"), &qbsError))
            return currentCommit;
        qDebug("Commit %s is not buildable.", qPrintable(currentCommit));
    }
    throw TestError(QString::fromLocal8Bit("Cannot run test: Failed to find a single commit that "
            "builds successfully with qbs. The last qbs error was: '%1'").arg(qbsError));
}

void FuzzyTester::runGit(const QStringList &arguments, QString *output)
{
    QProcess git;
    git.start("git", arguments);
    if (!git.waitForStarted())
        throw TestError("Failed to start git. It is expected to be in the PATH.");
    if (!git.waitForFinished(300000) || git.exitStatus() != QProcess::NormalExit) // 5 minutes ought to be enough for everyone
        throw TestError(QString::fromLocal8Bit("git failed: %1").arg(git.errorString()));
    if (git.exitCode() != 0) {
        throw TestError(QString::fromLocal8Bit("git failed: %1")
                        .arg(QString::fromLocal8Bit(git.readAllStandardError())));
    }
    if (output)
        *output = QString::fromLocal8Bit(git.readAllStandardOutput()).trimmed();
}

bool FuzzyTester::runQbs(const QString &buildDir, const QString &command, QString *errorOutput)
{
    if (errorOutput)
        errorOutput->clear();
    QProcess qbs;
    QStringList commandLine = QStringList(command) << "-qq" << "-d" << buildDir;
    if (m_jobCount != 0)
        commandLine << "--jobs" << QString::number(m_jobCount);
    commandLine << ("profile:" + m_profile);
    qbs.start("qbs", commandLine);
    if (!qbs.waitForStarted())
        throw TestError("Failed to start qbs. It is expected to be in the PATH.");
    if (!qbs.waitForFinished(-1) || qbs.exitCode() != 0) {
        if (errorOutput)
            *errorOutput = QString::fromLocal8Bit(qbs.readAllStandardError());
        return false;
    }
    return true;
}

void FuzzyTester::removeDir(const QString &dirPath)
{
    QDir dir(dirPath);
    if (!dir.removeRecursively()) {
        throw TestError(QString::fromLocal8Bit("Failed to remove temporary dir '%1'.")
                        .arg(dir.absolutePath()));
    }
}

bool FuzzyTester::doCleanBuild(QString *errorMessage)
{
    const QString cleanBuildDir = "fuzzytest-verification-build";
    removeDir(cleanBuildDir);
    return runQbs(cleanBuildDir, QLatin1String("build"), errorMessage);
}

void FuzzyTester::throwIncrementalBuildError(const QString &message,
                                             const QStringList &commitSequence)
{
    const QString commitSequenceString = commitSequence.join(QLatin1String(","));
    throw TestError(QString::fromLocal8Bit("Found qbs bug with incremental build!\n"
            "%1\n"
            "The sequence of commits was: %2.").arg(message, commitSequenceString));
}

QString FuzzyTester::defaultBuildDir()
{
    return "fuzzytest-build";
}
