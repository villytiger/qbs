import qbs 1.0

Product {
    type: "application"
    consoleApplication: true
    name : "app-and-lib-app"
    files : [ "main.d" ]
    Depends { name: "d" }
    Depends { name: "app-and-lib-lib" }
}
