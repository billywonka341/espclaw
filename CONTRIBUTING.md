# Contributing to espclaw

First off, thank you for considering contributing to `espclaw`! We want to make contributing to this project as easy and transparent as possible, whether it's:

- Reporting a bug
- Discussing the current state of the code
- Submitting a fix
- Proposing new features
- Becoming a maintainer

## We Develop with GitHub
We use GitHub to host code, to track issues and feature requests, as well as accept pull requests.

## How to Contribute

### 1. Create an Issue
Before making significant changes, please open an Issue to discuss your planned changes. This saves everyone time and ensures your work aligns with the project direction!

### 2. Fork & Create a Branch
Fork this repository and create your branch from `main`.
```bash
git checkout -b feature/my-amazing-feature
```

### 3. Setup the Local Environment
1. Install [PlatformIO](https://platformio.org/) to manage dependencies and compilations.
2. Ensure you have the required core files. `espclaw` compiles conditionally between `d1_mini` and `esp32dev` environments.
3. *Note:* Make sure to test your code on BOTH platforms (if your change is generic) or write `#ifdef ESP8266`/`#ifdef ESP32` guards if you are writing platform-specific features.
   ```bash
   pio run -e d1_mini
   pio run -e esp32dev
   ```

### 4. Make Your Changes
Write your code! Please ensure:
- Your core features are extremely memory efficient. If developing for the ESP8266, be hyper-aware of your RAM budget (you realistically only have ~40KB of free heap).
- Pre-allocate strings where possible or use stream parsers. We use `ArduinoJson 7` filtering directly on TCP sockets for a reason!

### 5. Format the Code
Make sure your C++ code adheres to standard formatting. (PlatformIO often has built in AST formatters you can use). Ensure it compiles cleanly without warnings.

### 6. Submit a Pull Request
1. Commit your changes:
   ```bash
   git commit -m "feat: Add My Amazing Feature"
   ```
2. Push your branch to your fork.
3. Open a Pull Request from your branch against our `main` branch.

All contributions are welcome, and we'll gladly work with you to get your PR merged!

## License
By contributing, you agree that your contributions will be licensed under its MIT License.
