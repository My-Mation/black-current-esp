# AI Technical Guide: ESP32 Test Station Audio Submission

This guide explains how audio recordings are captured, processed, and submitted to the Next.js backend.

## 1. Capture Flow
1. **Trigger**: Handled in `web_js.h` via `startRecording()`.
2. **Snapshots**: When recording begins, the system creates a "snapshot" of `curRootIndex` and `curIsFollowUp`. This prevents race conditions if the student advances the question while encoding is still happening.
3. **Encoding**: Audio is captured as WebM/Opus and converted to **MP3** (mono, 44100Hz, 128kbps) using `lamejs` in the browser.

## 2. Storage & Naming
Recordings are stored in the global `voiceBlobs` object using specific keys:
- **Main Question**: `${rootIndex}_m`
- **Follow-up Question**: `${rootIndex}_f`

Example: Question 5 (index 4) Main -> `4_m`.

## 3. Submission Format
Data is sent as `multipart/form-data` to `/api/submit`.

### Field Mapping Rules:
| Interaction Type | `voiceBlobs` Key | Multipart Field Name | Filename |
| :--- | :--- | :--- | :--- |
| Main question at index `i` | `i_m` | `audio_q{i}` | `audio_q{i}.mp3` |
| Follow-up at index `i` | `i_f` | `audio_q{i}_f` | `audio_q{i}_f.mp3` |

### Synchronization:
The `answers` JSON array must contain `null` for any question that has an attached audio file. The server uses the `rollNumber` and the field name (e.g., `audio_q4`) to save the file permanently.

## 4. Troubleshooting
- **Missing Audio**: Check if `pendingEncodings > 0` before submission. The system now has a wait loop in `finalizeSubmission`.
- **Wrong Question ID**: Ensure `isFollowUp` is correctly reported in the ESP32 `/api/state` poll.
