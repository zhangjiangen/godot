
echo scons platform=osx arch=x86_64 --jobs=$(sysctl -n hw.logicalcpu) vulkan=yes driver=Vulkan
scons platform=osx arch=x86_64 --jobs=$(sysctl -n hw.logicalcpu) vulkan=yes driver=Vulkan


echo cp -r misc/dist/osx_tools.app ./Godot.app
cp -r misc/dist/osx_tools.app ./Godot.app


echo mkdir -p Godot.app/Contents/MacOS
mkdir -p Godot.app/Contents/MacOS


echo cp bin/godot_osx_tools_x86_64 Godot.app/Contents/MacOS/Godot
chmod +x bin/godot_osx_tools_x86_64
cp bin/godot_osx_tools_x86_64 Godot.app/Contents/MacOS/Godot


echo chmod +x Godot.app/Contents/MacOS/Godot
chmod +x Godot.app/Contents/MacOS/Godot
