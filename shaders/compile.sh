#!/bin/bash

dir=`dirname $0`

/usr/bin/glslc $dir/shader.vert -o $dir/vert.spv
/usr/bin/glslc $dir/shader.frag -o $dir/frag.spv

/usr/bin/glslc $dir/second.vert -o $dir/second_vert.spv
/usr/bin/glslc $dir/second.frag -o $dir/second_frag.spv

