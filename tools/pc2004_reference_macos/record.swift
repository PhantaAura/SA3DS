import Foundation
import AppKit
import ScreenCaptureKit
import AVFoundation
guard CommandLine.arguments.count == 4,
      let pid = Int32(CommandLine.arguments[1]),
      let seconds = Double(CommandLine.arguments[3]), seconds > 0, seconds <= 1800 else {
 print("usage: record SONIC_MAC_PID /external/output.mp4 SECONDS (up to 1800)")
 exit(2)
}
let destination = URL(fileURLWithPath:CommandLine.arguments[2]).standardizedFileURL.resolvingSymlinksInPath()
var directory = destination.deletingLastPathComponent()
while directory.path != "/" {
 if FileManager.default.fileExists(atPath:directory.appendingPathComponent(".git").path) {
  print("Save private game recordings outside Git checkouts"); exit(2)
 }
 directory.deleteLastPathComponent()
}
guard !FileManager.default.fileExists(atPath:destination.path) else {
 print("Recording output already exists"); exit(2)
}
let application = NSApplication.shared
application.setActivationPolicy(.prohibited)
final class RecorderDelegate: NSObject, SCRecordingOutputDelegate {
 func recordingOutputDidStartRecording(_ recordingOutput: SCRecordingOutput) { print("RECORDING_STARTED: Sonic window only; no microphone or desktop"); fflush(stdout) }
 func recordingOutput(_ recordingOutput: SCRecordingOutput, didFailWithError error: Error) { print("RECORDING_FAILED: \(error)"); exit(1) }
 func recordingOutputDidFinishRecording(_ recordingOutput: SCRecordingOutput) { print("RECORDING_FINISHED"); fflush(stdout); exit(0) }
}
let delegate = RecorderDelegate()
var stream: SCStream?
Task {
 do {
  let content = try await SCShareableContent.excludingDesktopWindows(false,onScreenWindowsOnly:false)
  guard let window = content.windows.first(where: {$0.owningApplication?.processID == pid && $0.title == "SonicAdventureDXPC" && $0.frame.width > 100}) else { print("Sonic window unavailable"); exit(2) }
  let config = SCStreamConfiguration()
  config.width = Int(window.frame.width) / 2 * 2
  config.height = Int(window.frame.height) / 2 * 2
  config.minimumFrameInterval = CMTime(value:1,timescale:30)
  config.showsCursor = false
  config.capturesAudio = false
  let recording = SCRecordingOutputConfiguration()
  recording.outputURL = destination
  recording.videoCodecType = .h264
  recording.outputFileType = .mp4
  let output = SCRecordingOutput(configuration:recording,delegate:delegate)
  stream = SCStream(filter:SCContentFilter(desktopIndependentWindow:window),configuration:config,delegate:nil)
  try stream!.addRecordingOutput(output)
  try await stream!.startCapture()
  DispatchQueue.main.asyncAfter(deadline:.now()+seconds) {
   Task { do { try await stream!.stopCapture() } catch { print(error); exit(1) } }
  }
 } catch { print(error); exit(1) }
}
DispatchQueue.global().asyncAfter(deadline:.now()+seconds+20) { print("Recording timed out"); exit(124) }
application.run()
