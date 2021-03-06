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

#ifndef H_AVSM
#define H_AVSM

#include "AVSM_def.h"
#include "Common.hlsl"
#include "ListTexture.hlsl"

//////////////////////////////////////////////
// Defines
//////////////////////////////////////////////

#define AVSM_RT_COUNT (AVSM_NODE_COUNT / 4)

///////////////////////
// Structures
///////////////////////

struct AVSMData 
{
    float4 depth[AVSM_RT_COUNT];
    float4 trans[AVSM_RT_COUNT];
};

struct AVSMData_PSOut
{
    AVSMData Data : SV_Target;  // AVSM data
};

struct AVSMSegment
{
    int   index;
    float depthA;
    float depthB;
    float transA;
    float transB;
};

///////////////////////
// Constants
///////////////////////

cbuffer AVSMConstants
{   
    float4    mMask0;
    float4    mMask1;
    float4    mMask2;
    float4    mMask3;
    float4    mMask4;   
    float     mEmptyNode;
    float     mOpaqueNodeTrans;
    float     mShadowMapSize;       
}; 

//////////////////////////////////////////////
// AVSM Interpolation Functions
//////////////////////////////////////////////

float ExpInterp(float start, float end, float t)
{
	const float logBaseEulerConversion = 1.0/log(2.71828);
	const float minArg = 0.0001f;
	
	end   = max(end, minArg);
	start = max(start, minArg);
	
	float logEnd	  = log(end);	
	float logStart	  = log(start);		
	float linearValue = logStart + t * (logEnd - logStart);
	
	return exp(linearValue * logBaseEulerConversion);
}				

float Interp(float d0, float d1, float t0, float t1, float r)
{
    float depth = linstep(d0, d1, r);
#ifdef AVSM_LINEAR_SAMPLING
    return t0 + (t1 - t0) * depth;
#else
    return ExpInterp(t0, t1, depth);
#endif    
}

////////////////////////////////////////////////
// AVSM Error Metric And Compression Functions
////////////////////////////////////////////////

void ComputeCompIndices(in int i, 
                        out int a0, out int b0, 
                        out int a1, out int b1,
                        out int a2, out int b2)
{                        
    // looks like bad/slow code, but a good shader compiler will
    // fold all these computations into pre-computed indices
    a0 = (i + 0) >> 2;
    b0 = (i + 0) - (a0 << 2);
    a1 = (i + 1) >> 2;
    b1 = (i + 1) - (a1 << 2);
    a2 = (i + 2) >> 2;
    b2 = (i + 2) - (a2 << 2);
}    

// Computes the integral of the exponential function defined over
// the interval [d0, d1] and respectively interpolating [t0, t1]
float ExpArea(float d0, float d1, float t0, float t1)
{
	const float minArg = 0.0001f;
	t0 = max(t0, minArg);
	t1 = max(t1, minArg);

    float D = d1 - d0;
    float m = log(t1) - log(t0);
    
    return (D * t0) * (exp(m) - 1.0f) / max(m, minArg);
}

float TransmittanceAreaDelta(float d0, float d1, float d2, float t0, float t1, float t2)
{
	// Any internal node is connected to two segments, the integral of these 2 segments
	// can be computed as the sum of the area of two triangles.
	// From this sum we subtract the area of the single triangle generated by the node removal.
	
	// We don't need to take in account the integral of the rest of the transmittance curve as its delta
	// would be zero anyway.	
#ifdef AVSM_LINEAR_ERROR_METRIC	
	float areaDelta = (d2-d0)*(t2-t1) - (d2-d1)*(t2-t0);	
#else		
	float areaDelta = ExpArea(d0, d2, t0, t2) - ExpArea(d0, d1, t0, t1) - ExpArea(d1, d2, t1, t2);
#endif		

	// Given that we always want to work with positive values we compute the square of this
	// area delta. We don't care much about its value (per se) as long as we apply monotonic positive
	// functions to it. 
	return areaDelta * areaDelta; 
}

//////////////////////////////////////////////
// AVSM Insertion Functions
//////////////////////////////////////////////

AVSMSegment FindSegmentAVSM(in AVSMData data, in float receiverDepth)
{
    int    index;
    float4 depth, trans;
    float  leftDepth, rightDepth;
    float  leftTrans, rightTrans; 
    
    AVSMSegment Output;      

#if AVSM_RT_COUNT > 3    
    if (receiverDepth > data.depth[2][3])
    {
        depth        = data.depth[3];
        trans        = data.trans[3];
        leftDepth    = data.depth[2][3];
        leftTrans    = data.trans[2][3];
#if AVSM_RT_COUNT == 4
        rightDepth   = data.depth[3][3];        
        rightTrans   = data.trans[3][3];            
#else        
        rightDepth   = data.depth[4][0];        
        rightTrans   = data.trans[4][0];            
#endif        
        Output.index = 12;        
    }
    else
#endif    
#if AVSM_RT_COUNT > 2    
    if (receiverDepth > data.depth[1][3])
    {
        depth        = data.depth[2];
        trans        = data.trans[2];
        leftDepth    = data.depth[1][3];
        leftTrans    = data.trans[1][3];
#if AVSM_RT_COUNT == 3
        rightDepth   = data.depth[2][3];        
        rightTrans   = data.trans[2][3];            
#else        
        rightDepth   = data.depth[3][0];        
        rightTrans   = data.trans[3][0];            
#endif             
        Output.index = 8;        
    }
    else
#endif    
#if AVSM_RT_COUNT > 1    
    if (receiverDepth > data.depth[0][3])
    {
        depth        = data.depth[1];
        trans        = data.trans[1];
        leftDepth    = data.depth[0][3];
        leftTrans    = data.trans[0][3];
#if AVSM_RT_COUNT == 2
        rightDepth   = data.depth[1][3];        
        rightTrans   = data.trans[1][3];            
#else        
        rightDepth   = data.depth[2][0];        
        rightTrans   = data.trans[2][0];            
#endif        
        Output.index = 4;        
    }
    else
#endif
    {    
        depth        = data.depth[0];
        trans        = data.trans[0];
        leftDepth    = data.depth[0][0];
        leftTrans    = data.trans[0][0];
#if AVSM_RT_COUNT == 1
        rightDepth   = data.depth[0][3];        
        rightTrans   = data.trans[0][3];            
#else        
        rightDepth   = data.depth[1][0];        
        rightTrans   = data.trans[1][0];            
#endif        
        Output.index = 0;        
    } 
      

    if (receiverDepth > depth[3]) {
        Output.index += 4;       
        Output.depthA = depth[3];
        Output.depthB = rightDepth;
        Output.transA = trans[3];
        Output.transB = rightTrans;   
	} else if (receiverDepth > depth[2]) {
        Output.index += 3;    
        Output.depthA = depth[2];
        Output.depthB = depth[3];
        Output.transA = trans[2];
        Output.transB = trans[3];   
	} else if (receiverDepth > depth[1]) {
        Output.index += 2;
        Output.depthA = depth[1];
        Output.depthB = depth[2];
        Output.transA = trans[1];
        Output.transB = trans[2];   
	} else if (receiverDepth > depth[0]) {
        Output.index += 1;
        Output.depthA = depth[0]; 
        Output.depthB = depth[1];
        Output.transA = trans[0];
        Output.transB = trans[1];  
	} else {
        Output.depthA = leftDepth;
        Output.depthB = depth[0];
        Output.transA = leftTrans;
        Output.transB = trans[0];    
	}

    return Output;
}	

void RemoveFirstDataNode(inout float4 data[AVSM_RT_COUNT + 1], in float idx)
{
    data[0]   = mMask0 >= idx.xxxx ? float4(data[0].yzw, data[1].x) : data[0];
#if AVSM_RT_COUNT == 1    
    data[1].x = mMask1.x >= idx ? data[1].y : data[1].x;	
#endif    

#if AVSM_RT_COUNT > 1    
    data[1]   = mMask1 >= idx.xxxx ? float4(data[1].yzw, data[2].x) : data[1];     
#if AVSM_RT_COUNT == 2        
    data[2].x = mMask2.x >= idx ? data[2].y : data[2].x;	
#endif    
#endif    

#if AVSM_RT_COUNT > 2    
    data[2]   = mMask2 >= idx.xxxx ? float4(data[2].yzw, data[3].x) : data[2];     
#if AVSM_RT_COUNT == 3        
    data[3].x = mMask3.x >= idx ? data[3].y : data[3].x;	
#endif    
#endif    

#if AVSM_RT_COUNT > 3    
    data[3]   = mMask3 >= idx.xxxx ? float4(data[3].yzw, data[4].x) : data[3];     
#if AVSM_RT_COUNT == 4
    data[4].x = mMask4.x >= idx ? data[4].y : data[4].x;	
#endif    
#endif    
} 

void RemoveSecondDataNode(inout float4 data[AVSM_RT_COUNT + 1], in float idx)
{
    data[0]   = mMask0 >= idx.xxxx ? float4(data[0].yzw, data[1].x) : data[0];

#if AVSM_RT_COUNT > 1    
    data[1]   = mMask1 >= idx.xxxx ? float4(data[1].yzw, data[2].x) : data[1];     
#endif    

#if AVSM_RT_COUNT > 2    
    data[2]   = mMask2 >= idx.xxxx ? float4(data[2].yzw, data[3].x) : data[2];     
#endif    

#if AVSM_RT_COUNT > 3    
    data[3]   = mMask3 >= idx.xxxx ? float4(data[3].yzw, data[4].x) : data[3];     
#endif    
}

void StoreDataRep(inout AVSMData data, in float4 depth[AVSM_RT_COUNT + 1], in float4 trans[AVSM_RT_COUNT + 1])
{
    data.depth[0] = depth[0];
    data.trans[0] = trans[0];    
#if AVSM_RT_COUNT > 1    
    data.depth[1] = depth[1];
    data.trans[1] = trans[1];       
#endif     
#if AVSM_RT_COUNT > 2    
    data.depth[2] = depth[2];
    data.trans[2] = trans[2];       
#endif      
#if AVSM_RT_COUNT > 3    
    data.depth[3] = depth[3];
    data.trans[3] = trans[3];       
#endif         
}
		       
void InsertSegmentAVSM(in float segmentDepth[2], 
                       in float segmentTransmittance, 
                       inout AVSMData avsmData)
{	
    // find first depth node location within the current transmittance curve
    AVSMSegment seg0 = FindSegmentAVSM(avsmData, segmentDepth[0]);    
    // find last insertion slot	
    AVSMSegment seg1 = FindSegmentAVSM(avsmData, segmentDepth[1]); 

    // If transmittance is close to zero or smaller we don't need
    // to add any more data -> early exit test.								
    float minTransmittance = seg0.transA;      
#ifdef AVSM_EARLY_REJECTION            
    if (minTransmittance > mOpaqueNodeTrans)
#endif
    {		
	    int i,j,ci,cj,a0,b0,a1,b1,a2,b2;      

	    int preMoveSegmentEndIdx    = seg1.index;
	    int postMoveSegmentEndIdx   = seg1.index + 1;            
        int preMoveSegmentStartIdx  = seg0.index;
        int postMoveSegmentStartIdx = seg0.index;            
        float4 newDepth[AVSM_RT_COUNT + 1];
	    float4 newTrans[AVSM_RT_COUNT + 1];		
	        	    
		float depth[AVSM_NODE_COUNT+2];	
        float trans[AVSM_NODE_COUNT+2];	                
		[unroll] for (ci = 0; ci < 	AVSM_RT_COUNT; ++ci) {
		    [unroll] for (cj = 0; cj < 4; ++cj) {
		        depth[4 * ci + cj] = avsmData.depth[ci][cj];
		        trans[4 * ci + cj] = avsmData.trans[ci][cj];			        
		    }
		}		
		trans[AVSM_NODE_COUNT]   = FIRST_NODE_TRANS_VALUE;
		trans[AVSM_NODE_COUNT+1] = FIRST_NODE_TRANS_VALUE;
		depth[AVSM_NODE_COUNT]   = mEmptyNode;
		depth[AVSM_NODE_COUNT+1] = mEmptyNode;  	        
    
        // Fast path: post-curve insertion
#ifdef AVSM_FAST_PATHS        
	    if (mEmptyNode == seg0.depthB) { 					
   	    									     
            float updatedTrans = segmentTransmittance COMP_OP minTransmittance;			                                  
  			[unroll] for (i = 0; i < AVSM_NODE_COUNT + 2; ++i) {
                float d, t;
                int ki = i >> 2;
                int kj = i & 0x3;  
                           
                // Insert last segment node
                [flatten]if (i == 1 + postMoveSegmentStartIdx) {
                    d = segmentDepth[1];
                    t = updatedTrans;
                // Insert first segment node
                } else if (i == postMoveSegmentStartIdx) {
                    d = segmentDepth[0];
                    t = minTransmittance;                    
                // Update all nodes located in front the new two nodes          
                } else {
                    d = depth[i];
                    t = trans[i];
                }                
             
                // Generate new nodes
                newDepth[ki][kj] = d;
                newTrans[ki][kj] = t;
			}           
	    } else if (seg1.index == 0) {	                                  	
            // Fast path: pre-curve insertion	    
  			[unroll] for (i = 0; i < AVSM_NODE_COUNT + 2; ++i) 
			{
                float d, t;
                int ki = i >> 2;
                int kj = i & 0x3;  
                           
                // Insert last segment node
                [flatten]if (i == 1) {
                    d = segmentDepth[1];
                    t = segmentTransmittance;
                // Insert first segment node
                } else if (i == 0) {
                    d = segmentDepth[0];
                    t = FIRST_NODE_TRANS_VALUE;                    
                // Update all nodes located in front the new two nodes          
                } else {
                    d = depth[i-2];
                    t = segmentTransmittance COMP_OP trans[i-2];
                }                
             
                // Generate new nodes
                newDepth[ki][kj] = d;
                newTrans[ki][kj] = t;
			}    	    
	    }
	    else // Slow insertion path 
#endif	    
	    {	   	    
            // find last node that needs to be moved
            AVSMSegment segLast = FindSegmentAVSM(avsmData, mEmptyNode);          		    
		    int preMoveLastNodeToMove = segLast.index - 1;

		    // compute transmisttance offsets for new the new nodes
            float newNodesCdfOffset[2];
            newNodesCdfOffset[0] = Interp(seg0.depthA, seg0.depthB, seg0.transA, seg0.transB, segmentDepth[0]);		     
            newNodesCdfOffset[1] = Interp(seg1.depthA, seg1.depthB, seg1.transA, seg1.transB, segmentDepth[1]);	  

#ifdef AVSM_DYNAMIC_INDEXING			    
		    // move post-segment nodes and update their cdf
		    for(i = preMoveLastNodeToMove; i >= preMoveSegmentEndIdx; --i) {		
			    trans[2 + i] = trans[i] COMP_OP segmentTransmittance;
			    depth[2 + i] = depth[i];
		    }	
    					
		    //move intra-segment nodes and update their cdf
		    for(i = preMoveSegmentEndIdx; i >= preMoveSegmentStartIdx; --i) {
			    trans[1 + i] = trans[i] COMP_OP Interp(segmentDepth[0], segmentDepth[1], FIRST_NODE_TRANS_VALUE, segmentTransmittance, depth[i]);
			    depth[1 + i] = depth[i];
		    }			
               	
		    // insert end of the segment node
		    depth[postMoveSegmentEndIdx] = segmentDepth[1];
		    trans[postMoveSegmentEndIdx] = segmentTransmittance COMP_OP newNodesCdfOffset[1]; 
    		
		    // insert start of the segment node
		    depth[postMoveSegmentStartIdx] = segmentDepth[0];
		    trans[postMoveSegmentStartIdx] = newNodesCdfOffset[0];      		
    
		    {
		        // Copy avsm data from indexable arrays of scalar values to vectors
		        // We could still perform the rest of the job over the indexable arrays
		        // but working on vectors generates faster code.    		    
		        int loadIdx;
		        [unroll] for(int ci = 0; ci < AVSM_RT_COUNT; ++ci) {
		            loadIdx = 4 * ci;
		            newDepth[ci] = float4(depth[loadIdx], depth[1 + loadIdx], depth[2 + loadIdx], depth[3 + loadIdx]);
		            newTrans[ci] = float4(trans[loadIdx], trans[1 + loadIdx], trans[2 + loadIdx], trans[3 + loadIdx]);		            
		        } 		  
		        
		        loadIdx = 4 * AVSM_RT_COUNT;
		        newDepth[AVSM_RT_COUNT] = float4(depth[loadIdx], depth[1 + loadIdx], mEmptyNode, mEmptyNode);		        
		        newTrans[AVSM_RT_COUNT] = float4(trans[loadIdx], trans[1 + loadIdx], trans[1 + loadIdx], trans[1 + loadIdx]);		        
		    }    	
#else		    	
            // segment insertion implementation with no dynamic indexing            	        		
			[unroll] for (i = 0; i < AVSM_NODE_COUNT + 2; ++i) {
                float d, t;
                int ki = i >> 2;
                int kj = i & 0x3;  
                           
                // Insert last segment node
                [flatten]if (i == postMoveSegmentEndIdx) {
                    d = segmentDepth[1];
                    t = newNodesCdfOffset[1];
                // Insert first segment node
                } else if (i == postMoveSegmentStartIdx) {
                    d = segmentDepth[0];
                    t = newNodesCdfOffset[0];                    
                // Update all nodes in between the new two nodes
                } else if ((i > postMoveSegmentStartIdx) && (i < postMoveSegmentEndIdx)) {
                    d = depth[i-1];
                    t = trans[i-1];    
                // Update all nodes located behind the new two nodes          
                } else if ((i > 1) && (i > postMoveSegmentEndIdx)) {
                    d = depth[i-2];
                    t = trans[i-2];
                // Update all nodes located in front the new two nodes          
                } else {
                    d = depth[i];
                    t = trans[i];
                }
                
                // Transmittance compositing
                t = t COMP_OP Interp(segmentDepth[0], segmentDepth[1], FIRST_NODE_TRANS_VALUE, segmentTransmittance, d);
             
                // Generate new nodes
                newDepth[ki][kj] = d;
                newTrans[ki][kj] = t;
			}    		
#endif    		
    	}

        // pack representation if we have too many nodes
        [flatten]if (newDepth[AVSM_RT_COUNT][0] != mEmptyNode) {	        
         
	        int numNodes = AVSM_NODE_COUNT + 2;
	        // for each internal node compute the difference
	        // between the integral of the CDF before and after the removal
	        float area[AVSM_NODE_COUNT];				 
            [unroll]for (i = 0; i < numNodes - 2; ++i) {
                ComputeCompIndices(i, a0, b0, a1, b1, a2, b2);	                
                area[i] = TransmittanceAreaDelta(newDepth[a0][b0], 
                                                 newDepth[a1][b1], 
                                                 newDepth[a2][b2],
				    					         newTrans[a0][b0], 
											     newTrans[a1][b1], 
											     newTrans[a2][b2]);									    
            }		        
    		
	        // find the node that generates 
	        // the smallest delta in the CDF integral
	        int areaToRemoveIdx = 0;
	        float smallest = area[0];
	        [unroll]for (i = 0; i < numNodes - 2; ++i) {
		        if (smallest > area[i]) {
			        smallest = area[i];
			        areaToRemoveIdx = i;
		        }
	        }         
	        
	        // remove the first node (depth and trasmittance)        		
            float ridx = 1.0f + (float)areaToRemoveIdx;		
	        RemoveFirstDataNode(newDepth, ridx);
	        RemoveFirstDataNode(newTrans, ridx);	         		              		
	        		               		
	        // We have just removed one node..
	        numNodes--;		
            
            // recompute nodes metric (it's faster to recompute them all!)
            [unroll]for (i = 0; i < numNodes - 2; ++i) {
                ComputeCompIndices(i, a0, b0, a1, b1, a2, b2);	                
                area[i] = TransmittanceAreaDelta(newDepth[a0][b0], 
                                                 newDepth[a1][b1], 
                                                 newDepth[a2][b2],
				    					         newTrans[a0][b0], 
											     newTrans[a1][b1], 
											     newTrans[a2][b2]);									    
            }	

	        // find the node that generates  
	        // the smallest delta in the CDF integral
	        areaToRemoveIdx = 0;
	        smallest = area[0];
	        [unroll]for (i = 0; i < numNodes - 2; ++i) {
		        if (smallest > area[i]) {
			        smallest = area[i];
			        areaToRemoveIdx = i;
		        }
	        }	
	        
	        // remove the second node
            ridx = 1.0f + (float)areaToRemoveIdx;		
            RemoveSecondDataNode(newDepth, ridx);
            RemoveSecondDataNode(newTrans, ridx);               
	    }		

	    //////////////////////////////////////////
	    // Store AVSM reprsentation 		
	    StoreDataRep(avsmData, newDepth, newTrans);    
    }    
}		


//////////////////////////////////////////////
// AVSM Sampling Functions
//////////////////////////////////////////////

SamplerState   gAVSMSampler;
Texture2DArray gAVSMTexture;

void LoadDataLevel(inout AVSMData data, in float2 uv, in float mipLevel)
{
#if AVSM_RT_COUNT > 3
    data.depth[3] = gAVSMTexture.Sample(gAVSMSampler, float3(uv, 3));
    data.trans[3] = gAVSMTexture.Sample(gAVSMSampler, float3(uv, 3 + AVSM_RT_COUNT));
#endif
#if AVSM_RT_COUNT > 2
    data.depth[2] = gAVSMTexture.Sample(gAVSMSampler, float3(uv, 2));
    data.trans[2] = gAVSMTexture.Sample(gAVSMSampler, float3(uv, 2 + AVSM_RT_COUNT));
#endif
#if AVSM_RT_COUNT > 1
    data.depth[1] = gAVSMTexture.Sample(gAVSMSampler, float3(uv, 1));
    data.trans[1] = gAVSMTexture.Sample(gAVSMSampler, float3(uv, 1 + AVSM_RT_COUNT));
#endif
    data.depth[0] = gAVSMTexture.Sample(gAVSMSampler, float3(uv, 0));
    data.trans[0] = gAVSMTexture.Sample(gAVSMSampler, float3(uv, 0 + AVSM_RT_COUNT));
}

float EvalAbsTransmittance(in AVSMData avsmData, in float receiverDepth)
{
    AVSMSegment seg = FindSegmentAVSM(avsmData, receiverDepth);
    return Interp(seg.depthA, seg.depthB, seg.transA, seg.transB, receiverDepth);
}

float AVSMPointSample(in float2 uv, in float receiverDepth)
{
    AVSMData avsmData;
    LoadDataLevel(avsmData, uv, 0.0f);
    
    return EvalAbsTransmittance(avsmData, receiverDepth);
}

float AVSMBilinearSample(in float2 textureCoords, in float receiverDepth)
{
    float2 unnormCoords = textureCoords * mShadowMapSize.xx;

    const float a = frac(unnormCoords.x - 0.5f);
    const float b = frac(unnormCoords.y - 0.5f);    
    const float i = floor(unnormCoords.x - 0.5f);
    const float j = floor(unnormCoords.y - 0.5f);
    
    float sample00 = AVSMPointSample(float2(i, j)         / mShadowMapSize.xx, receiverDepth);
    float sample01 = AVSMPointSample(float2(i, j + 1)     / mShadowMapSize.xx, receiverDepth);
    float sample10 = AVSMPointSample(float2(i + 1, j)     / mShadowMapSize.xx, receiverDepth);
    float sample11 = AVSMPointSample(float2(i + 1, j + 1) / mShadowMapSize.xx, receiverDepth);              
	
	return (1 - a)*(1 - b)*sample00 + a*(1-b)*sample10 + (1-a)*b*sample01 + a*b*sample11;
}
// Generalized volume sampling function
// TODO: move to Common.hlsl
float VolumeSample(in uint method, in float2 textureCoords, in float receiverDepth)
{

#ifdef SAMPLING_NO_SWITCHCASE
#ifdef ENABLE_AVSM_SAMPLING    
#ifdef AVSM_BILINEARF        
    return AVSMBilinearSample(textureCoords, receiverDepth);                                         
#else
    return AVSMPointSample(textureCoords, receiverDepth);                                         
#endif            
#endif

#ifdef ENABLE_AVSM_GAUSS7X7_SAMPLING    
    float2 unnormCoords = textureCoords * mShadowMapSize.xx;

    const float gauss[5][5] = {
    1, 4, 7, 4, 1,
    4, 16, 26, 16, 4,
    7, 26, 41, 26, 7,
    4, 16, 26, 16, 4,
    1, 4, 7, 4, 1
    };

    float shadowing = 0.0f;
    for (int i = -2; i <= 2; ++i) {
        for (int j = -2; j <= 2; ++j) {
            float2 uv = (unnormCoords + float2(i, j)) / mShadowMapSize.xx;
            float s = AVSMBilinearSample(uv, receiverDepth);
            shadowing += s * gauss[i+2][j+2] / 273.0f;
        }
    }
    return shadowing;
#endif


#else  
    switch (method) { 
#ifdef ENABLE_AVSM_SAMPLING    
        case(VOL_SHADOW_AVSM):
        case(VOL_SHADOW_AVSM_CS):
        case(VOL_SHADOW_AVSM_RTR):
#ifdef AVSM_BILINEARF        
            return AVSMBilinearSample(textureCoords, receiverDepth);                                         
#else
            return AVSMPointSample(textureCoords, receiverDepth);                                         
#endif            
#endif


#ifdef ENABLE_AVSM_GAUSS7X7_SAMPLING    
        case(VOL_SHADOW_AVSM_GAUSS_7): {
        
	        float2 unnormCoords = textureCoords * mShadowMapSize.xx;

	        const float gauss[5][5] = {
		    1, 4, 7, 4, 1,
		    4, 16, 26, 16, 4,
		    7, 26, 41, 26, 7,
		    4, 16, 26, 16, 4,
		    1, 4, 7, 4, 1
	        };

	        float shadowing = 0.0f;
	        for (int i = -2; i <= 2; ++i) {
		        for (int j = -2; j <= 2; ++j) {
		            float2 uv = (unnormCoords + float2(i, j)) / mShadowMapSize.xx;
		            float s = AVSMBilinearSample(uv, receiverDepth);
		            shadowing += s * gauss[i+2][j+2] / 273.0f;
		        }
	        }
	        return shadowing;
	      
	        
	        // see Ignacio Castano's blog: http://castano.ludicon.com/blog/2009/03/30/high-quality-shadow-filtering/
/*	        const float3 coeffs[9] = { {-1.619141, -1.619141, 0.021700},
                                       {0.000000, -1.619141, 0.103909},
                                       {1.619141, -1.619141, 0.021700},
                                       {-1.619141, 0.000000, 0.103909},
                                       {0.000000, 0.000000, 0.497567},
                                       {1.619141, 0.000000, 0.103909},
                                       {-1.619141, 1.61914, 0.021700},
                                       {0.000000, 1.619141, 0.103909},
                                       {1.619141, 1.619141, 0.021700} };
                                       
                                      
              const float3 coeffs2[16] = {   {-2.953125, -2.953125, 0.048133},
                                             {-0.984375, -2.953125, 0.061563},
                                             {0.984375, -2.953125, 0.061563},
                                             {2.953125, -2.953125, 0.048133},
                                             {-2.953125, -0.984375, 0.061563},
                                             {-0.984375, -0.984375, 0.078741},
                                             {0.984375, -0.984375, 0.078741},
                                             {2.953125, -0.984375, 0.061563},
                                             {-2.953125, 0.984375, 0.061563},
                                             {-0.984375, 0.984375, 0.078741},
                                             {0.984375, 0.984375, 0.078741},
                                             {2.953125, 0.984375, 0.061563},
                                             {-2.953125, 2.953125, 0.048133},
                                             {-0.984375, 2.953125, 0.061563},
                                             {0.984375, 2.953125, 0.061563},
                                             {2.953125, 2.953125, 0.048133}};
                                         
            float transmittance = 0;
            for (int i = 0; i < 9; ++i) {
                transmittance += coeffs[i].z * AVSMBilinearSample(textureCoords + (coeffs[i].xy / mShadowMapSize.xx) , receiverDepth); 
            }                                     
	        
	        return transmittance; */
	    }
#endif
	  
#ifdef ENABLE_AVSM_BOX4X4_SAMPLING
        case(VOL_SHADOW_AVSM_BOX_4):  {
                // 4x4 Box filter
	        float2 unnormCoords = textureCoords * mShadowMapSize.xx;
	        float2 fracCoords	= frac(unnormCoords); 
	        float2 intCoords	= floor(unnormCoords);
    	        
                float2 uv;
                uv = (float2(intCoords.x - 0.5, intCoords.y - 0.5) + fracCoords) /
                        mShadowMapSize.xx;
                    float s0 = AVSMBilinearSample(uv, receiverDepth);                                         
	            uv = (float2(intCoords.x - 0.5, intCoords.y + 0.5) + fracCoords) /
                        mShadowMapSize.xx;
                    float s1 = AVSMBilinearSample(uv, receiverDepth);                                         
	            uv = (float2(intCoords.x + 0.5, intCoords.y + 0.5) + fracCoords) /
                        mShadowMapSize.xx;
                    float s2 = AVSMBilinearSample(uv, receiverDepth);                                         
	            uv = (float2(intCoords.x + 0.5, intCoords.y - 0.5) + fracCoords) /
                        mShadowMapSize.xx;
                    float s3 = AVSMBilinearSample(uv, receiverDepth);                                         
	            return (s0 + s1 + s2 + s3) / 4.0f;
	    }
#endif	    

        default:
            return 1.0f;
    }      
#endif  // SAMPLING_NO_SWITCHCASE            
}

//////////////////////////////////////////////
// AVSM Helper Functions
//////////////////////////////////////////////
AVSMData AVSMGetEmptyNode()
{
    AVSMData empty;
    
    empty.depth[0] = float4(mEmptyNode, mEmptyNode, mEmptyNode, mEmptyNode);
    empty.trans[0] = float4(1, 1, 1, 1);    
#if AVSM_RT_COUNT > 1    
    empty.depth[1] = float4(mEmptyNode, mEmptyNode, mEmptyNode, mEmptyNode);
    empty.trans[1] = float4(1, 1, 1, 1);    
#endif
#if AVSM_RT_COUNT > 2    
    empty.depth[2] = float4(mEmptyNode, mEmptyNode, mEmptyNode, mEmptyNode);
    empty.trans[2] = float4(1, 1, 1, 1);    
#endif
#if AVSM_RT_COUNT > 3    
    empty.depth[3] = float4(mEmptyNode, mEmptyNode, mEmptyNode, mEmptyNode);
    empty.trans[3] = float4(1, 1, 1, 1);    
#endif
    
    return empty;
}    


////////////////////////////////////
// Debug shaders
////////////////////////////////////

float4 VisualizeListTexFirstNodeOffsetPS(FullScreenTriangleVSOut input) : SV_Target
{
    const float adjustRatio = mShadowMapSize / 256.0f;
    float x = adjustRatio * (input.positionViewport.x - 10.0f);
    float y = adjustRatio * (input.positionViewport.y - (720.0f - 256.0f - 10.0f));

    uint offset = LT_GetFirstSegmentNodeOffset(int2(x,y));   
    return offset != NODE_LIST_NULL ? float4(0, 0, 0, 0) : float4(1, 1, 1, 1);    
}  

Texture2DArray<float4>  gDebugAVSMDataSRV;

float4 VisualizeAVSMPS(FullScreenTriangleVSOut input) : SV_Target
{
    int2 viewportPos = int2(input.positionViewport.x,
                            input.positionViewport.y);

    float minTransmittance = 1.0f;
    for (int i = 0; i < AVSM_RT_COUNT; ++i) {
        float4 depth = gAVSMTexture[int3(viewportPos.xy,i)];
        float4 trans = gAVSMTexture[int3(viewportPos.xy,i + AVSM_RT_COUNT)];

        for (int j = 0; j < 4; ++j) {
            if (depth[j] != mEmptyNode) {
                minTransmittance = min(minTransmittance, trans[j]);
            }
        }
    }

    return minTransmittance;
}  

#endif // H_AVSM

