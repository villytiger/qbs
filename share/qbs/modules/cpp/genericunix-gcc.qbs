import qbs.base 1.0

UnixGCC {
    condition: qbs.targetOS === 'unix' && qbs.toolchain === 'gcc'
}