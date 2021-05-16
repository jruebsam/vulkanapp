#version 450

layout(input_attachment_index=0, binding=0) uniform subpassInput inputColour;
layout(input_attachment_index=1, binding=1) uniform subpassInput inputDepth;

layout(location=0) out vec4 colour;

void main()
{
  int xHalf = 1680/2;
  if(gl_FragCoord.x > xHalf)
  {
    float lowerbound = 0.98;
    float upperbound = 1;
    float depth = subpassLoad(inputDepth).r;
    float depthColorScale = 1.0f - ((depth - lowerbound)/ (upperbound - lowerbound));
    colour = vec4(subpassLoad(inputColour).rgb*depthColorScale,  1.0f);
  }

  else
  {
    colour = subpassLoad(inputColour).rgba;

  }

}

