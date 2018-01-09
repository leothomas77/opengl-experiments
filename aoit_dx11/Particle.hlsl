// Copyright 2011 Intel Corporation
// All Rights Reserved
//
// Permission is granted to use, copy, distribute and prepare derivative works of this
// software for any purpose and without fee, provided, that the above copyright notice
// and this statement appear in all copies.  Intel makes no representations about the
// suitability of this software for any purpose.  THIS SOFTWARE IS PROVIDED "AS IS."
// INTEL SPECIFICALLY DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED, AND ALL LIABILITY,
// INCLUDING CONSEQUENTIAL AND OTHER INDIRECT DAMAGES, FOR THE USE OF THIS SOFTWARE,
// INCLUDING LIABILITY FOR INFRINGEMENT OF ANY PROPRIETARY RIGHTS, AND INCLUDING THE
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  Intel does not
// assume any responsibility for any errors which may appear in this software nor any
// responsibility to update it.

#ifndef COMMON_PARTICLE_FX_H
#define COMMON_PARTICLE_FX_H

#include "AOIT.hlsl"
#include "AVSM_Resolve.hlsl"
#include "Rendering.hlsl"
#include "ConstantBuffers.hlsl"

//--------------------------------------------------------------------------------------

struct DynamicParticlePSIn
{
    float4 Position  : SV_POSITION;
    float3 UVS		 : TEXCOORD0;
    float  Opacity	 : TEXCOORD1;
    float3 ViewPos	 : TEXCOORD2;
    float3 ObjPos    : TEXCOORD3;
    float3 ViewCenter: TEXCOORD4;
};

//////////////////////////////////////////////
// Resource Views
//////////////////////////////////////////////
RWStructuredBuffer<AVSMData>  gAVSMStructBufUAV;
StructuredBuffer<AVSMData>    gAVSMStructBufSRV;
Texture2D                     gParticleOpacityNoiseTex;
       
bool RaySphereIntersection(out float2 interCoeff, in float3 sphereCenter, in float sphereRadius, in float3 rayOrigin, in float3 rayNormDir)
{
	float3 dst = rayOrigin - sphereCenter;
	float b = dot(dst, rayNormDir);
	float c = dot(dst, dst) - (sphereRadius * sphereRadius);
	float d = b * b - c;
	interCoeff = -b.xx + sqrt(d) * float2(-1,1);
	return d > 0;
}

// compute particles thickness assuming their billboards
// in screen space are maximal sections of a sphere.
float ParticleThickness(float2 ray)
{
	return saturate(1 - sqrt(2.0f) * length(ray));
}

// We model particles as small spheres and we intersect them with light rays
// Entry and exit points are used to defined an AVSM segment
bool IntersectDynamicParticle(in  DynamicParticlePSIn input,
                              out float3 entry,
                              out float3 exit,
                              out float  transmittance)
{
    bool   res;
    float2 linearCoeff;    
    float3 normRayDir     = normalize(input.ViewPos);
    float3 particleCenter = input.ViewCenter;
    float  particleSize = input.UVS.z;
    float  particleRadius = mScale * particleSize / 2.0f;
                
    [flatten]if (RaySphereIntersection(linearCoeff, 
                                       particleCenter, 
                                       particleRadius, 
                                       float3(0, 0, 0), 
                                       normRayDir)) {  
        // compute entry and exit points along the ray direction
	    entry = linearCoeff.xxx * normRayDir;  
	    exit  = linearCoeff.yyy * normRayDir;
	    
	    // compute normalized opacity
	    float segLength = (exit.z - entry.z) / (2.0f * particleRadius); 
	    // Compute density based indirectly on distance from center
	    float densityTimesSegLength = pow(segLength, 4.0f);
	    // per particle and global opacity multipliers
        float opacity = input.Opacity * mParticleOpacity;	
        // compute transmittance
	    transmittance = exp(-opacity * densityTimesSegLength);          	    	    
	    res = true;  
    } else {
        entry = 0;
        exit  = 0;
        transmittance = 1;        
        res = false;
    }     

    return res;
}                   

float GetDynamicParticleAlpha(DynamicParticlePSIn Input)
{    
    float3 entry, exit;
    float  transmittance;
    IntersectDynamicParticle(Input, entry, exit, transmittance);
    
    return saturate(transmittance);
}

//--------------------------------------------------------------------------------------
DynamicParticlePSIn DynamicParticlesShading_VS(
    float4 inPosition	: POSITION,
    float3 inUV			: TEXCOORD0,
    float  inOpacity	: TEXCOORD1
)
{
    DynamicParticlePSIn	Out;
    float size		= inUV.z * mParticleSize;

    // Make screen-facing
    float4 position;
    float2 offset	= inUV.xy - 0.5f.xx;
    position.xyz	= inPosition.xyz + size * (offset.xxx * mEyeRight.xyz + offset.yyy * mEyeUp.xyz);  
    position.w		= 1.0;

    float4 projectedPosition = mul( position, mParticleWorldViewProj );
    
    Out.Position    = projectedPosition;
    
    Out.ObjPos      = position.xyz;
    Out.ViewPos 	= mul( position, mParticleWorldView ).xyz;
    Out.ViewCenter	= mul( float4(inPosition.xyz, 1.0f), mParticleWorldView).xyz;
    Out.UVS			= float3(inUV.xy, size);
    Out.Opacity		= inOpacity;

    return Out;
}

SurfaceData ConstructSurfaceData(float3 PosView, float3 Normal)
{
    SurfaceData Surface;
    Surface.positionView = PosView;
    Surface.positionViewDX = ddx(Surface.positionView);// TODO: Which gradient should we use here?
    Surface.positionViewDY = ddy(Surface.positionView);// TODO: Which gradient should we use here?
    Surface.normal = Normal;
    Surface.albedo = float4(0,0,0,1);
    Surface.lightSpaceZ = mul(float4(Surface.positionView.xyz, 1.0f), mCameraViewToLightProj).z;
    Surface.lightTexCoord = ProjectIntoLightTexCoord(Surface.positionView.xyz);
    Surface.lightTexCoordDX = ddx(Surface.lightTexCoord); // TODO: Use gradient based on particle!
    Surface.lightTexCoordDY = ddy(Surface.lightTexCoord); // TODO: Use gradient based on particle!

    return Surface;
}

float ShadowContrib(SurfaceData LitSurface, DynamicParticlePSIn Input)
{
    float2 lightTexCoord = ProjectIntoAvsmLightTexCoord(LitSurface.positionView.xyz);
    float receiverDepth = mul(float4(LitSurface.positionView.xyz, 1.0f), mCameraViewToAvsmLightView).z;     
    
    return VolumeSample(mUI.volumeShadowMethod, lightTexCoord, receiverDepth);  
}

float4 ShadeParticle(DynamicParticlePSIn Input)
{
    float3 entry, exit;	
	float  shadowTerm = 1.0f;
	float  segmentTransmittance = 1.0f;
    [flatten]if (IntersectDynamicParticle(Input, entry, exit, segmentTransmittance)) {
	    float2 lightTexCoord = ProjectIntoLightTexCoord(entry);	    
	    float receiver = mul(float4(entry, 1.0f), mCameraViewToLightProj).z;  
			
        SurfaceData LitSurface = ConstructSurfaceData(entry, 0.0f.xxx);	
        if (mUI.enableVolumeShadowLookup) {
            shadowTerm = ShadowContrib(LitSurface, Input);
        }
    }

	float depthDiff = 1.0f;

    float3 normalView  = normalize(entry - Input.ViewCenter);
    float3 normalWorld = mul(float4(normalView, 0),  mCameraViewToWorld).xyz;

	float3 diffuse = float3(0.8,0.8,1.0); //saturate(dot(-mLightDir.xyz, normalView));
    float3 ambient = float3(0.6,0.6,0.8);

    ambient = gIBLTexture.Sample(gIBLSampler, normalWorld).xyz;
    ambient = ambient * 0.7.xxx + 0.3.xxx * gIBLTexture.Sample(gIBLSampler, -normalWorld).xyz;

    /* TODO: re-enable once the transition from deferred to forward rendering is complete
    [flatten]if (mbSoftParticles) {		
		// Calcualte the difference in the depths		
		SurfaceData SurfData = ComputeSurfaceDataFromGBuffer(int2(Input.Position.xy));
		
		// If the depth read-in is zero, there was nothing rendered at that pixel.
		// In such a case, we set depthDiff to 1.0f.
		if (SurfData.positionView.z == 0) {
			depthDiff = 1.0f;
		} else {
			depthDiff = (SurfData.positionView.z - Input.ViewPos.z) / mSoftParticlesSaturationDepth;
			depthDiff = smoothstep(0.0f, 1.0f, depthDiff);
		}
	}*/
  
    float3 Color = saturate(ambient * 0.05f.xxx + diffuse * shadowTerm.xxx);
    return float4(Color, depthDiff * (1.0f - segmentTransmittance));   
}

float4 DynamicParticlesShading_PS(DynamicParticlePSIn Input) : SV_Target
{
    return ShadeParticle(Input);
}

[earlydepthstencil]
void DynamicParticlesShading_Capture_PS(DynamicParticlePSIn Input, uint coverageMask : SV_Coverage) 
{
    uint newNodeAddress;       
    float4 particleColor = ShadeParticle(Input);   

    if (0 != particleColor.w) {
        // Get fragment viewport coordinates
        int2 screenAddress = int2(Input.Position.xy);            

#ifdef LIST_EARLY_DEPTH_CULL
        uint packedCullData = gFragmentListDepthCullUAV[screenAddress];
        float cullDepth = asfloat(packedCullData & 0xFFFFFF00UL);
        uint  cullTrans = packedCullData & 0xFFUL;
      	
        if ((cullTrans > EARLY_DEPTH_CULL_ALPHA) || (cullDepth > Input.ViewPos.z)) 
        {	
#endif        
            if (FL_AllocNode(newNodeAddress)) {
                // Fill node
                FragmentListNode node;            
                node.color    = FL_PackColor(particleColor);
                node.depth    = FL_PackDepthAndCoverage(Input.ViewPos.z, coverageMask);       
                        
                // Insert node!
                FL_InsertNode(screenAddress, newNodeAddress, node);            

#ifdef LIST_EARLY_DEPTH_CULL
                // Update culling data
                float trans = (1.0f - particleColor.w) * float(cullTrans);
                packedCullData = asuint(max(Input.ViewPos.z, cullDepth)) & 0xFFFFFF00UL;
                packedCullData = packedCullData | (uint(ceil(trans)) & 0xFFUL);
                gFragmentListDepthCullUAV[screenAddress] = packedCullData;
            }  
#endif 
        }
    }              
}

[earlydepthstencil]
void ParticleAVSMCapture_PS(DynamicParticlePSIn Input)
{
    float3 entry, exit;
    float  segmentTransmittance;
	if (IntersectDynamicParticle(Input, entry, exit, segmentTransmittance)) {
	
        // Allocate a new node
        // (If we're running out of memory we simply drop this fragment
        uint newNodeAddress;       
        
        if (LT_AllocSegmentNode(newNodeAddress)) {
            // Fill node
            ListTexSegmentNode node;            
            node.depth[0] = entry.z;
            node.depth[1] = exit.z;
            node.trans    = segmentTransmittance;
            node.sortKey  = Input.ViewPos.z;
            
	        // Get fragment viewport coordinates
            int2 screenAddress = int2(Input.Position.xy);            
            
            // Insert node!
            LT_InsertFirstSegmentNode(screenAddress, newNodeAddress, node);            
        }           
	} 
}


void AVSMClearStructuredBuf_PS(FullScreenTriangleVSOut Input)
{
    // Compute linearized address of this pixel
    uint2 screenAddress = uint2(Input.positionViewport.xy);
    uint address = screenAddress.y * (uint)(mShadowMapSize) + screenAddress.x; 

    // Initialize AVSM data (clear)
    gAVSMStructBufUAV[address] = AVSMGetEmptyNode();
}

#endif // COMMON_PARTICLE_FX_H
