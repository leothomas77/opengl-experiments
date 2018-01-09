float4 DrawTransmittanceVS(float3 pos : POSITION) : SV_Position {
    // Pass through
    return float4(pos, 1);
}

float4 DrawTransmittancePS() : SV_Target {
    return float4(1, 0, 0, 1);
}
