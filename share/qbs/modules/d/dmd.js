var File = loadExtension("qbs.File");
var FileInfo = loadExtension("qbs.FileInfo");
var ModUtils = loadExtension("qbs.ModUtils");
var PathTools = loadExtension("qbs.PathTools");
var UnixUtils = loadExtension("qbs.UnixUtils");
var WindowsUtils = loadExtension("qbs.WindowsUtils");

function prepareLinker(project, product, inputs, outputs, input, output) {
    var i, primaryOutput, cmd, commands = [], args = [];

    primaryOutput = outputs.application[0];

    args.push(input.filePath);
    args.push("-of" + output.filePath);

    cmd = new Command(ModUtils.moduleProperty(product, "linkerPath"), args);
    cmd.description = 'linking ' + output.fileName;
    cmd.highlight = 'linker';
    cmd.responseFileUsagePrefix = '@';

    return cmd;
}
