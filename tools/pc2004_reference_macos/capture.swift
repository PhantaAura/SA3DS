// Capture only the named Sonic process's game window; never activate it or send input.
import Foundation
import AppKit
import ScreenCaptureKit
import ImageIO
import UniformTypeIdentifiers

let application = NSApplication.shared
application.setActivationPolicy(.prohibited)
guard CommandLine.arguments.count == 3,
      let pid = Int32(CommandLine.arguments[1]) else {
    print("usage: capture SONIC_MAC_PID /external/output.png")
    exit(2)
}
let output = URL(fileURLWithPath: CommandLine.arguments[2]).standardizedFileURL.resolvingSymlinksInPath()
var directory = output.deletingLastPathComponent()
while directory.path != "/" {
    if FileManager.default.fileExists(atPath: directory.appendingPathComponent(".git").path) {
        print("Save private game captures outside Git checkouts")
        exit(2)
    }
    directory.deleteLastPathComponent()
}
DispatchQueue.global().asyncAfter(deadline: .now() + 15) {
    print("Sonic window capture timed out")
    exit(124)
}
Task {
    do {
        let content = try await SCShareableContent.excludingDesktopWindows(false, onScreenWindowsOnly: false)
        let windows = content.windows.filter {
            $0.owningApplication?.processID == pid &&
            $0.title == "SonicAdventureDXPC" && $0.frame.width > 100 && $0.frame.height > 100
        }
        guard let window = windows.max(by: {
            $0.frame.width * $0.frame.height < $1.frame.width * $1.frame.height
        }) else { print("No shareable Sonic game window for this PID"); exit(2) }
        let config = SCStreamConfiguration()
        config.width = Int(window.frame.width)
        config.height = Int(window.frame.height)
        config.showsCursor = false
        let filter = SCContentFilter(desktopIndependentWindow: window)
        let frame = try await SCScreenshotManager.captureImage(contentFilter: filter, configuration: config)
        guard let dest = CGImageDestinationCreateWithURL(output as CFURL, UTType.png.identifier as CFString, 1, nil)
        else { print("Cannot create output image"); exit(3) }
        CGImageDestinationAddImage(dest, frame, nil)
        guard CGImageDestinationFinalize(dest) else { print("Cannot finish output image"); exit(3) }
        print("Captured Sonic window \(window.windowID): \(frame.width)x\(frame.height)")
        exit(0)
    } catch { print("Sonic window capture failed: \(error)"); exit(1) }
}
application.run()
