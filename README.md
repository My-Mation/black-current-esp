# 🎓 AI-Powered Adaptive Testing Station (ESP32)

Welcome to the **AI-Powered Adaptive Testing Station** — a cutting-edge hardware and software ecosystem designed to revolutionize offline, personalized assessments. By combining the power of **NotebookLLM**, modern web technologies (Next.js), and robust ESP32 hardware, this project automates the creation, administration, and evaluation of student quizzes, significantly reducing the workload on educators.

---

## 🌟 The Vision
Teachers often spend countless hours creating, conducting, and evaluating personalized quizzes for individual students. This project streamlines the entire workflow:
1. **Upload & Generate**: Teachers (or students) upload study materials (PDFs) to a web portal.
2. **AI Processing**: NotebookLLM analyzes the materials and automatically generates a dynamic quiz in JSON format.
3. **Offline Administration**: The ESP32 Test Station receives the quiz and administers it to students interactively, accommodating multiple students at once.
4. **Smart Evaluation**: Instead of just returning a raw score, NotebookLLM analyzes the submitted answers and provides targeted feedback on weak points and topics to improve.

---

## 🚀 Key Features

### 🧠 NotebookLLM Integration
- **Automated Quiz Generation**: Upload any PDF to the web portal. The system seamlessly communicates with NotebookLLM to scrape and generate a structured JSON quiz.
- **Dynamic Question Types**: The engine supports a diverse set of questions:
  - Multiple Choice Questions (MCQ) & True/False
  - Numeric Input (Numbers)
  - Fill in the Blanks
  - Voice/Audio Responses (Type the actual answer via voice)
- **Personalized AI Feedback**: Upon submission, responses are analyzed by NotebookLLM. The AI returns a detailed JSON report highlighting the student's specific weak topics and areas for improvement, offering a truly adaptive learning experience.

### 💻 Robust Software Architecture
- **Next.js Backend**: Handles the queueing of test responses, media processing, and communication with NotebookLLM.
- **Adaptive Test Engine**: The C++ ESP32 firmware features an adaptive state machine (`TestState`). Questions can have "follow-up" nodes, creating branching paths based on the student's previous answers.
- **Background Syncing**: Exam responses (including multipart/form-data audio blobs) are queued and securely synced to the backend without interrupting the user experience.
- **Audio Encoding**: Captures WebM/Opus audio and intelligently encodes it to MP3 (mono, 44100Hz, 128kbps) for reliable backend storage.

### 🛠️ Interactive Hardware Interface (ESP32)
The physical test station is built on an ESP32 dual-core processor and features a rich multi-modal interface:
- **OLED Display (`oled_handler`)**: Provides crisp visual feedback, displaying questions, test status, and WiFi connection details.
- **TM1637 Timer (`tm1637_handler`)**: A dedicated 7-segment display keeps track of the elapsed test time, keeping students aware of their pacing.
- **Matrix Keypad (`keypad_handler`)**: Allows students to input numeric answers and navigate multiple-choice options effortlessly.
- **Touch Sensor (`touch`)**: An intuitive touch-to-record interface for capturing voice answers, complete with multi-stage touch detection (Start, Stop, Confirm).
- **Buzzer Feedback (`buzzer`)**: Delivers auditory cues for system boot, key presses, and test completion.

---

## 🔄 The Complete Workflow

1. **Preparation Phase**
   - Study materials (PDFs) are uploaded to the NotebookLLM scraper website.
   - NotebookLLM parses the context and generates a dynamic quiz JSON containing MCQs, numeric questions, and voice prompts.
   - The JSON payload is sent via a `POST` request to the ESP32.

2. **Testing Phase**
   - The ESP32 parses the JSON and initializes the `TestState` engine.
   - The student inputs their Roll Number using the keypad.
   - The test begins: The OLED displays the questions, and the TM1637 timer starts ticking.
   - Depending on the question type:
     - **MCQ**: Student presses A, B, C, or D on the keypad.
     - **Numeric**: Student types the exact number and submits.
     - **Voice**: Student uses the touch sensor to record their spoken answer.
   - *Adaptive Branching*: Based on the answer, the engine may trigger a follow-up question to probe deeper into the student's understanding.

3. **Submission & Analysis Phase**
   - Once all questions are answered, the ESP32 compiles the interactions and audio files.
   - The data is sent as `multipart/form-data` to the Next.js backend queue.
   - The backend forwards the responses to NotebookLLM.
   - **Feedback Loop**: NotebookLLM returns a comprehensive feedback JSON detailing the student's weak points, which is then made available for the student and teacher to review.

---

## 🛠️ Technical Stack
- **Hardware**: ESP32 (Dual-Core), OLED I2C Display, TM1637 7-Segment Display, 4x4 Matrix Keypad, Touch Sensor, Active Buzzer.
- **Firmware**: C++, PlatformIO, Arduino framework, ArduinoJson.
- **Backend/Web**: Next.js, HTML5/CSS/Vanilla JS (for embedded web server), Lamejs (MP3 encoding).
- **AI Engine**: Google NotebookLLM.

---

## 💡 Why This Matters
By offloading the repetitive tasks of test creation and evaluation to AI, and digitizing the offline test-taking experience with affordable hardware, educators can focus on what truly matters: **mentoring and guiding students through their personalized learning journeys.**
