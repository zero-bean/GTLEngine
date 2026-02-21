// A simple depth-only pixel shader. It doesn't output any color,
// only writes to the depth buffer based on the vertex shader output.
// If the hardware supports depth-only rendering without a pixel shader,
// this can be omitted.
void main()
{
    // No color output, just let the depth buffer be written.
}
