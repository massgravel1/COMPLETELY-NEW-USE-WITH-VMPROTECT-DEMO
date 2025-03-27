# SSS-AND-LOFE-CLOUD DLL Injector

A powerful DLL injector with driver support for bypassing anti-cheat protections.

## Features

- Standard manual mapping injection
- Kernel-mode driver-based injection (bypasses most anti-cheat protections)
- Select target process by name, ID, or from a list
- Specify custom DLL path
- Target window by class name
- Automatic fallback to standard injection if driver isn't available

## Building

To build the injector:

```bash
nob
```

This will create `injector.exe` in the current directory.

## Usage

```
Usage: injector.exe [options]
Options:
  --help, -h           Show this help message
  --process, -p NAME   Specify process name (default: osu!.exe)
  --dll, -d PATH       Specify DLL path
  --pid, -i PID        Specify process ID directly
  --class, -c NAME     Specify window class name
  --list, -l           Select process from list
  --driver, -r         Force use of driver (if available)
```

### Examples

Inject the default DLL into osu!.exe:
```
injector.exe
```

Inject a specific DLL:
```
injector.exe --dll path\to\your\dll.dll
```

Inject into a specific process:
```
injector.exe --process notepad.exe
```

Inject into a process with a specific window class:
```
injector.exe --class "Notepad"
```

Select process from a list:
```
injector.exe --list
```

## Driver Usage

The injector includes a kernel-mode driver that enhances injection capabilities by bypassing certain anti-cheat protections. The driver is extracted and installed automatically when needed.

**Note**: Using the driver requires administrative privileges. You must run the injector as administrator for the driver to work properly.

To force the use of the driver:
```
injector.exe --driver
```

## Important Notes

- The driver is loaded, used, and then unloaded automatically to minimize detection
- You need administrator privileges to use the driver feature
- Some antivirus software may detect the driver as malicious (false positive)
- Always use this tool responsibly and only on your own systems or with proper authorization

## Advanced Usage

For advanced users who wish to modify the driver or customize the injection process, you can replace the placeholder driver binary in `driver/driver_binary.h` with your own driver's binary data.

## Credits

- Original injector code from SSS-AND-LOFE-CLOUD project
