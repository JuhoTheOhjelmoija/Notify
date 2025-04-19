# Clipboard Monitor

A lightweight Windows application that notifies you when content is copied to the clipboard.

## Features

- Monitors clipboard changes
- Shows a notification when text is copied to the clipboard
- Runs in the system tray
- Only visible as a system tray icon
- Lightweight and fast
- Low memory usage
- Unicode support
- Modern C++ implementation
- No external dependencies

## Requirements

- Windows 10 or later
- MinGW compiler for building

## Building

Compile the program using MinGW with the following command:

```bash
x86_64-w64-mingw32-g++ -o ClipboardMonitor.exe clipboard_notifier.cpp -static -static-libgcc -static-libstdc++ -mwindows -luser32 -lgdi32 -lshell32
```

## Usage

1. Compile the program using the command above
2. Run `ClipboardMonitor.exe`
3. The program will appear only in the system tray
4. When you copy text to the clipboard, you'll see a notification
5. To exit, right-click the system tray icon and select "Exit"

## Notes

- The program only works on Windows
- You need MinGW compiler to build the program
- The program doesn't store clipboard content, it only shows notifications about changes

## License

MIT License - see LICENSE file for details

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request. 