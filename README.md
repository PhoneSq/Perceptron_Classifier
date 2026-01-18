🟢 Perceptron Classifier

A minimalist Perceptron-based binary classifier with retro Win32 UI – classify files like spam vs. ham using keyword features, all in C++ with a green-on-black terminal aesthetic.

🚀 Features

Custom Perceptron Logic
Configurable input size, learning rate, and bias.
Step activation function & score computation.
Save/load trained models for persistent use.
Interactive Win32 GUI

Train Mode: feed sample files with labels, train for N epochs, and watch logs update in real-time.
Use Mode: classify new files with instant predictions.
Retro “console” look: green text on black background with scrollable logs.
Keyword-Based Feature Extraction
Default spam keywords: free, win, money, offer, click, buy, urgent, etc.
Easily extendable for custom datasets or additional features.
Clean, Modular C++ Code
Separates perceptron logic, feature extraction, and UI.
Minimal dependencies, easy to build and extend.

🛠️ Getting Started

Open the project in Visual Studio (Win32 API).
Build and run the executable.
Train Mode: input sample files, labels, and epochs.
Use Mode: classify files using trained models.
Recommended: start with 3 spam + 3 ham files to test training flow.

📂 Project Structure

Perceptron.h / Perceptron.cpp — core perceptron class and training logic
FeatureExtractor.h / FeatureExtractor.cpp — file parsing & keyword feature extraction
Main.cpp — Win32 GUI, state machine, and logging

🔮 Future Enhancements
Add dynamic keyword loading for flexible datasets
Multi-class classification support
Polished Use Mode UI with live feature display

💻 Requirements

Windows OS
Visual Studio (Win32 API support)
C++17 or later

