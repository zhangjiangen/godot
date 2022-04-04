#!/bin/sh

#  post_build.sh
#  godot4
#
#  Created by zhangjiangen on 2022/4/5.
#

echo ln -f ../bin/godot.osx.tools.x86_64 ../bin/godot4
echo $(pwd)
cd $(dirname $0)
echo $(pwd)
cd ../bin
ls
echo ln -f godot_osx_tools_x86_64 godot4
ln -f godot_osx_tools_x86_64 godot4
chmod -R 777 godot4
echo finish!!
