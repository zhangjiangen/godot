
echo scons platform=osx arch=x86_64 --jobs=$(sysctl -n hw.logicalcpu)
scons platform=osx arch=x86_64 --jobs=$(sysctl -n hw.logicalcpu)


echo cp -r misc/dist/osx_tools.app ./Godot.app
cp -r misc/dist/osx_tools.app ./Godot.app


echo mkdir -p Godot.app/Contents/MacOS
mkdir -p Godot.app/Contents/MacOS


echo cp bin/godot.osx.tools.x86_64 Godot.app/Contents/MacOS/Godot
cp bin/godot.osx.tools.x86_64 Godot.app/Contents/MacOS/Godot


echo chmod +x Godot.app/Contents/MacOS/Godot
chmod +x Godot.app/Contents/MacOS/Godot
