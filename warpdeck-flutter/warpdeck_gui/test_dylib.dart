import 'dart:ffi';
import 'dart:io';

void main() {
  try {
    print('Platform: ${Platform.operatingSystem}');
    print('Executable: ${Platform.resolvedExecutable}');
    
    final lib = DynamicLibrary.open('/Users/jesse/code/WarpDeck/warpdeck-flutter/warpdeck_gui/build/macos/Build/Products/Release/warpdeck_gui.app/Contents/MacOS/libwarpdeck.dylib');
    print('Library loaded successfully');
    
    // Try to lookup the function
    final createFunc = lib.lookup('warpdeck_create');
    print('Function found: $createFunc');
  } catch (e) {
    print('Error: $e');
  }
}