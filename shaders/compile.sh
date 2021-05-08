#!/bin/bash

dir=`dirname $0`

/usr/bin/glslc $dir/shader.vert -o $dir/vert.spv
/usr/bin/glslc $dir/shader.frag -o $dir/frag.spv


