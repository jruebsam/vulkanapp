#!/bin/bash

dir=`dirname $0`

/usr/bin/glslc $dir/shader.vert -o vert.spv
/usr/bin/glslc $dir/shader.frag -o frag.spv


