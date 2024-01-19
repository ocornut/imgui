// swift-tools-version:5.9

import PackageDescription

let package = Package(
    name: "DearIMGUITest",
    platforms: [
       .macOS(.v10_13)
    ],
    products: [
        .library(name: "DearIMGUI", targets: ["DearIMGUI"]),
        .executable(name: "DearIMGUIExample", targets: ["DearIMGUIExample"])
    ],
    targets: [
        .target(
            name: "DearIMGUI",
            dependencies: ["CSDL"]
        ),
        .executableTarget(
            name: "DearIMGUIExample",
            dependencies: [
                "DearIMGUI",
                "CSDL"
            ],
            swiftSettings: [
                .interoperabilityMode(.Cxx),
                .unsafeFlags(["-warnings-as-errors"])
            ],
            linkerSettings: [
                .linkedLibrary("GL", .when(platforms: [.linux])),
            ]
        ),
        .systemLibrary(
            name: "CSDL",
            pkgConfig: "sdl2"
        )
    ],
    cxxLanguageStandard: .cxx11
)
