/*
Copyright (c) 2006, Michael Kazhdan and Matthew Bolitho
All rights reserved.

Contains modifications by Simon Fuhrmann, 2013.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list of
conditions and the following disclaimer. Redistributions in binary form must reproduce
the above copyright notice, this list of conditions and the following disclaimer
in the documentation and/or other materials provided with the distribution.

Neither the name of the Johns Hopkins University nor the names of its contributors
may be used to endorse or promote products derived from this software without specific
prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
TO, PROCUREMENT OF SUBSTITUTE  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
DAMAGE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <algorithm>

////////////////////////
// OctNode::NodeIndex //
////////////////////////
template<class NodeData,class Real>
OctNode<NodeData,Real>::NodeIndex::NodeIndex(void)
{
    depth=offset[0]=offset[1]=offset[2]=0;
}
template<class NodeData,class Real>
typename OctNode<NodeData,Real>::NodeIndex OctNode<NodeData,Real>::NodeIndex::child(const int& cIndex) const
{
    int x,y,z;
    NodeIndex idx;
    Cube::FactorCornerIndex(cIndex,x,y,z);
    idx.depth=depth+1;
    idx.offset[0]=(offset[0]<<1)|x;
    idx.offset[1]=(offset[1]<<1)|y;
    idx.offset[2]=(offset[2]<<1)|z;
    return idx;
}
template<class NodeData,class Real>
typename OctNode<NodeData,Real>::NodeIndex OctNode<NodeData,Real>::NodeIndex::parent(void) const
{
    NodeIndex idx;
    idx.depth=depth-1;
    idx.offset[0]=offset[0]>>1;
    idx.offset[1]=offset[1]>>1;
    idx.offset[2]=offset[2]>>1;
    return idx;
}
template<class NodeData,class Real>
typename OctNode<NodeData,Real>::NodeIndex& OctNode<NodeData,Real>::NodeIndex::operator += (const int& cIndex)
{
    int x,y,z;
    Cube::FactorCornerIndex(cIndex,x,y,z);
    depth++;
    offset[0]=(offset[0]<<1)|x;
    offset[1]=(offset[1]<<1)|y;
    offset[2]=(offset[2]<<1)|z;
    return *this;
}
template<class NodeData,class Real>
typename OctNode<NodeData,Real>::NodeIndex& OctNode<NodeData,Real>::NodeIndex::operator -- (void)
{
    depth--;
    offset[0]>>=1;
    offset[1]>>=1;
    offset[2]>>=1;
    return *this;
}

/////////////
// OctNode //
/////////////

template <class NodeData,class Real>
OctNode<NodeData,Real>::OctNode(void)
{
    parent=children=NULL;
}

template <class NodeData,class Real>
OctNode<NodeData,Real>::~OctNode(void)
{
    if(children){delete[] children;}
    parent=children=NULL;
}

template <class NodeData,class Real>
int OctNode<NodeData,Real>::initChildren(void)
{
    int i,j,k;

    if(children){delete[] children;}
    children=NULL;
    children=new OctNode[Cube::CORNERS];
    if(!children){
        fprintf(stderr,"Failed to initialize children in OctNode::initChildren\n");
        exit(0);
        return 0;
    }
    for(i=0;i<2;i++){
        for(j=0;j<2;j++){
            for(k=0;k<2;k++){
                int idx=Cube::CornerIndex(i,j,k);
                children[idx].parent=this;
                children[idx].children=NULL;
            }
        }
    }
    return 1;
}

template <class NodeData,class Real>
void OctNode<NodeData,Real>::deleteChildren(void)
{
    if(children)
        delete[] children;
    children=NULL;
}

template <class NodeData,class Real>
inline void OctNode<NodeData,Real>::CenterAndWidth(const NodeIndex &nIndex,VertexType& center,Real& width)
{
    width=Real(1.0/(1<<nIndex.depth));
    for(int dim=0;dim<3;dim++)
        center[dim]=Real(0.5+nIndex.offset[dim])*width;
}

template <class NodeData,class Real>
int OctNode<NodeData,Real>::maxDepth(void) const
{
    if(!children){return 0;}
    else{
        int c,d;
        for(int i=0;i<Cube::CORNERS;i++){
            d=children[i].maxDepth();
            if(!i || d>c){c=d;}
        }
        return c+1;
    }
}

template <class NodeData,class Real>
int OctNode<NodeData,Real>::nodes(void) const
{
    if(!children){return 1;}
    else{
        int c=0;
        for(int i=0;i<Cube::CORNERS;i++){c+=children[i].nodes();}
        return c+1;
    }
}

template <class NodeData,class Real>
int OctNode<NodeData,Real>::leaves(void) const
{
    if(!children){return 1;}
    else{
        int c=0;
        for(int i=0;i<Cube::CORNERS;i++){c+=children[i].leaves();}
        return c;
    }
}

template<class NodeData,class Real>
int OctNode<NodeData,Real>::maxDepthLeaves(const int& maxDepth) const
{
    //if(depth>maxDepth){return 0;}  // was: depth()
    if(!children){return 1;}
    else{
        int c=0;
        for(int i=0;i<Cube::CORNERS;i++){c+=children[i].maxDepthLeaves(maxDepth);}
        return c;
    }
}

template <class NodeData,class Real>
const OctNode<NodeData,Real>* OctNode<NodeData,Real>::root(void) const
{
    const OctNode* temp=this;
    while(temp->parent){temp=temp->parent;}
    return temp;
}

template <class NodeData,class Real>
const OctNode<NodeData,Real>* OctNode<NodeData,Real>::nextBranch(const OctNode* current) const
{
    if(!current->parent || current==this){return NULL;}
    if(current-current->parent->children==Cube::CORNERS-1){return nextBranch(current->parent);}
    else{return current+1;}
}

template <class NodeData,class Real>
OctNode<NodeData,Real>* OctNode<NodeData,Real>::nextBranch(OctNode* current)
{
    if(!current->parent || current==this){return NULL;}
    if(current-current->parent->children==Cube::CORNERS-1){return nextBranch(current->parent);}
    else{return current+1;}
}

template <class NodeData,class Real>
const OctNode<NodeData,Real>* OctNode<NodeData,Real>::nextLeaf(const OctNode* current) const
{
    if(!current)
    {
        const OctNode<NodeData,Real>* temp=this;
        while(temp->children){temp=&temp->children[0];}
        return temp;
    }
    if(current->children){return current->nextLeaf(NULL);}
    const OctNode* temp=nextBranch(current);
    if(!temp){return NULL;}
    else{return temp->nextLeaf(NULL);}
}

template <class NodeData,class Real>
OctNode<NodeData,Real>* OctNode<NodeData,Real>::nextLeaf(OctNode* current)
{
    if(!current)
    {
        OctNode<NodeData,Real>* temp=this;
        while(temp->children){temp=&temp->children[0];}
        return temp;
    }
    if(current->children){return current->nextLeaf(NULL);}
    OctNode* temp=nextBranch(current);
    if(!temp){return NULL;}
    else{return temp->nextLeaf(NULL);}
}

template <class NodeData,class Real>
const OctNode<NodeData,Real>* OctNode<NodeData,Real>::nextNode(const OctNode* current) const
{
    if(!current){return this;}
    else if(current->children){return &current->children[0];}
    else{return nextBranch(current);}
}

template <class NodeData,class Real>
OctNode<NodeData,Real>* OctNode<NodeData,Real>::nextNode(OctNode* current)
{
    if(!current){return this;}
    else if(current->children){return &current->children[0];}
    else{return nextBranch(current);}
}

template <class NodeData,class Real>
const OctNode<NodeData,Real>* OctNode<NodeData,Real>::nextBranch(const OctNode* current,NodeIndex& nIndex) const
{
    if(!current->parent || current==this)
        return NULL;
    int c=current-current->parent->children;
    nIndex--;
    if(c==Cube::CORNERS-1)
        return nextBranch(current->parent,nIndex);
    else
    {
        nIndex+=c;
        return current+1;
    }
}

template <class NodeData,class Real>
OctNode<NodeData,Real>* OctNode<NodeData,Real>::nextBranch(OctNode* current,NodeIndex& nIndex)
{
    if(!current->parent || current==this)
        return NULL;
    int c=int(current-current->parent->children);
    --nIndex;
    if(c==Cube::CORNERS-1)
        return nextBranch(current->parent,nIndex);
    else
    {
        nIndex+=c+1;
        return current+1;
    }
}

template <class NodeData,class Real>
const OctNode<NodeData,Real>* OctNode<NodeData,Real>::nextLeaf(const OctNode* current,NodeIndex& nIndex) const
{
    if(!current)
    {
        const OctNode<NodeData,Real>* temp=this;
        while(temp->children)
        {
            nIndex+=0;
            temp=&temp->children[0];
        }
        return temp;
    }
    if(current->children)
        return current->nextLeaf(NULL,nIndex);
    const OctNode* temp=nextBranch(current,nIndex);
    if(!temp)
        return NULL;
    else
        return temp->nextLeaf(NULL,nIndex);
}

template <class NodeData,class Real>
OctNode<NodeData,Real>* OctNode<NodeData,Real>::nextLeaf(OctNode* current,NodeIndex& nIndex)
{
    if(!current){
        OctNode<NodeData,Real>* temp=this;
        while(temp->children)
        {
            nIndex+=0;
            temp=&temp->children[0];
        }
        return temp;
    }
    if(current->children)
        return current->nextLeaf(NULL,nIndex);
    OctNode* temp=nextBranch(current,nIndex);
    if(!temp)
        return NULL;
    else
        return temp->nextLeaf(NULL,nIndex);
}

template <class NodeData,class Real>
const OctNode<NodeData,Real>* OctNode<NodeData,Real>::nextNode(const OctNode* current,NodeIndex& nIndex) const
{
    if(!current)
        return this;
    else if(current->children)
    {
        nIndex+=0;
        return &current->children[0];
    }
    else
        return nextBranch(current,nIndex);
}

template <class NodeData,class Real>
OctNode<NodeData,Real>* OctNode<NodeData,Real>::nextNode(OctNode* current,NodeIndex& nIndex)
{
    if(!current)
        return this;
    else if(current->children)
    {
        nIndex+=0;
        return &current->children[0];
    }
    else
        return nextBranch(current,nIndex);
}

template <class NodeData,class Real>
void OctNode<NodeData,Real>::setFullDepth(const int& maxDepth)
{
    if(maxDepth){
        if(!children){initChildren();}
        for(int i=0;i<8;i++){children[i].setFullDepth(maxDepth-1);}
    }
}

template <class NodeData,class Real>
OctNode<NodeData,Real>* OctNode<NodeData,Real>::faceNeighbor(const int& faceIndex,const int& forceChildren)
{
    return __faceNeighbor(faceIndex>>1,faceIndex&1,forceChildren);
}

template <class NodeData,class Real>
const OctNode<NodeData,Real>* OctNode<NodeData,Real>::faceNeighbor(const int& faceIndex) const
{
    return __faceNeighbor(faceIndex>>1,faceIndex&1);
}

template <class NodeData,class Real>
OctNode<NodeData,Real>* OctNode<NodeData,Real>::__faceNeighbor(const int& dir,const int& off,const int& forceChildren)
{
    if(!parent){return NULL;}
    int pIndex=int(this-parent->children);
    pIndex^=(1<<dir);
    if((pIndex & (1<<dir))==(off<<dir)){return &parent->children[pIndex];}
//	if(!(((pIndex>>dir)^off)&1)){return &parent->children[pIndex];}
    else{
        OctNode* temp=parent->__faceNeighbor(dir,off,forceChildren);
        if(!temp){return NULL;}
        if(!temp->children){
            if(forceChildren){temp->initChildren();}
            else{return temp;}
        }
        return &temp->children[pIndex];
    }
}

template <class NodeData,class Real>
const OctNode<NodeData,Real>* OctNode<NodeData,Real>::__faceNeighbor(const int& dir,const int& off) const {
    if(!parent){return NULL;}
    int pIndex=int(this-parent->children);
    pIndex^=(1<<dir);
    if((pIndex & (1<<dir))==(off<<dir)){return &parent->children[pIndex];}
//	if(!(((pIndex>>dir)^off)&1)){return &parent->children[pIndex];}
    else{
        const OctNode* temp=parent->__faceNeighbor(dir,off);
        if(!temp || !temp->children){return temp;}
        else{return &temp->children[pIndex];}
    }
}

template <class NodeData,class Real>
OctNode<NodeData,Real>* OctNode<NodeData,Real>::edgeNeighbor(const int& edgeIndex,const int& forceChildren){
    int idx[2],o,i[2];
    Cube::FactorEdgeIndex(edgeIndex,o,i[0],i[1]);
    switch(o){
        case 0:	idx[0]=1;	idx[1]=2;	break;
        case 1:	idx[0]=0;	idx[1]=2;	break;
        case 2:	idx[0]=0;	idx[1]=1;	break;
    };
    return __edgeNeighbor(o,i,idx,forceChildren);
}

template <class NodeData,class Real>
const OctNode<NodeData,Real>* OctNode<NodeData,Real>::edgeNeighbor(const int& edgeIndex) const {
    int idx[2],o,i[2];
    Cube::FactorEdgeIndex(edgeIndex,o,i[0],i[1]);
    switch(o){
        case 0:	idx[0]=1;	idx[1]=2;	break;
        case 1:	idx[0]=0;	idx[1]=2;	break;
        case 2:	idx[0]=0;	idx[1]=1;	break;
    };
    return __edgeNeighbor(o,i,idx);
}

template <class NodeData,class Real>
const OctNode<NodeData,Real>* OctNode<NodeData,Real>::__edgeNeighbor(const int& o,const int i[2],const int idx[2]) const{
    if(!parent){return NULL;}
    int pIndex=int(this-parent->children);
    int aIndex,x[3];

    Cube::FactorCornerIndex(pIndex,x[0],x[1],x[2]);
    aIndex=(~((i[0] ^ x[idx[0]]) | ((i[1] ^ x[idx[1]])<<1))) & 3;
    pIndex^=(7 ^ (1<<o));
    if(aIndex==1)	{	// I can get the neighbor from the parent's face adjacent neighbor
        const OctNode* temp=parent->__faceNeighbor(idx[0],i[0]);
        if(!temp || !temp->children){return NULL;}
        else{return &temp->children[pIndex];}
    }
    else if(aIndex==2)	{	// I can get the neighbor from the parent's face adjacent neighbor
        const OctNode* temp=parent->__faceNeighbor(idx[1],i[1]);
        if(!temp || !temp->children){return NULL;}
        else{return &temp->children[pIndex];}
    }
    else if(aIndex==0)	{	// I can get the neighbor from the parent
        return &parent->children[pIndex];
    }
    else if(aIndex==3)	{	// I can get the neighbor from the parent's edge adjacent neighbor
        const OctNode* temp=parent->__edgeNeighbor(o,i,idx);
        if(!temp || !temp->children){return temp;}
        else{return &temp->children[pIndex];}
    }
    else{return NULL;}
}

template <class NodeData,class Real>
OctNode<NodeData,Real>* OctNode<NodeData,Real>::__edgeNeighbor(const int& o,const int i[2],const int idx[2],const int& forceChildren){
    if(!parent){return NULL;}
    int pIndex=int(this-parent->children);
    int aIndex,x[3];

    Cube::FactorCornerIndex(pIndex,x[0],x[1],x[2]);
    aIndex=(~((i[0] ^ x[idx[0]]) | ((i[1] ^ x[idx[1]])<<1))) & 3;
    pIndex^=(7 ^ (1<<o));
    if(aIndex==1)	{	// I can get the neighbor from the parent's face adjacent neighbor
        OctNode* temp=parent->__faceNeighbor(idx[0],i[0],0);
        if(!temp || !temp->children){return NULL;}
        else{return &temp->children[pIndex];}
    }
    else if(aIndex==2)	{	// I can get the neighbor from the parent's face adjacent neighbor
        OctNode* temp=parent->__faceNeighbor(idx[1],i[1],0);
        if(!temp || !temp->children){return NULL;}
        else{return &temp->children[pIndex];}
    }
    else if(aIndex==0)	{	// I can get the neighbor from the parent
        return &parent->children[pIndex];
    }
    else if(aIndex==3)	{	// I can get the neighbor from the parent's edge adjacent neighbor
        OctNode* temp=parent->__edgeNeighbor(o,i,idx,forceChildren);
        if(!temp){return NULL;}
        if(!temp->children){
            if(forceChildren){temp->initChildren();}
            else{return temp;}
        }
        return &temp->children[pIndex];
    }
    else{return NULL;}
}

template <class NodeData,class Real>
const OctNode<NodeData,Real>* OctNode<NodeData,Real>::cornerNeighbor(const int& cornerIndex) const {
    int pIndex,aIndex=0;
    if(!parent){return NULL;}

    pIndex=int(this-parent->children);
    aIndex=(cornerIndex ^ pIndex);	// The disagreement bits
    pIndex=(~pIndex)&7;				// The antipodal point
    if(aIndex==7){					// Agree on no bits
        return &parent->children[pIndex];
    }
    else if(aIndex==0){				// Agree on all bits
        const OctNode* temp=((const OctNode*)parent)->cornerNeighbor(cornerIndex);
        if(!temp || !temp->children){return temp;}
        else{return &temp->children[pIndex];}
    }
    else if(aIndex==6){				// Agree on face 0
        const OctNode* temp=((const OctNode*)parent)->__faceNeighbor(0,cornerIndex & 1);
        if(!temp || !temp->children){return NULL;}
        else{return & temp->children[pIndex];}
    }
    else if(aIndex==5){				// Agree on face 1
        const OctNode* temp=((const OctNode*)parent)->__faceNeighbor(1,(cornerIndex & 2)>>1);
        if(!temp || !temp->children){return NULL;}
        else{return & temp->children[pIndex];}
    }
    else if(aIndex==3){				// Agree on face 2
        const OctNode* temp=((const OctNode*)parent)->__faceNeighbor(2,(cornerIndex & 4)>>2);
        if(!temp || !temp->children){return NULL;}
        else{return & temp->children[pIndex];}
    }
    else if(aIndex==4){				// Agree on edge 2
        const OctNode* temp=((const OctNode*)parent)->edgeNeighbor(8 | (cornerIndex & 1) | (cornerIndex & 2) );
        if(!temp || !temp->children){return NULL;}
        else{return & temp->children[pIndex];}
    }
    else if(aIndex==2){				// Agree on edge 1
        const OctNode* temp=((const OctNode*)parent)->edgeNeighbor(4 | (cornerIndex & 1) | ((cornerIndex & 4)>>1) );
        if(!temp || !temp->children){return NULL;}
        else{return & temp->children[pIndex];}
    }
    else if(aIndex==1){				// Agree on edge 0
        const OctNode* temp=((const OctNode*)parent)->edgeNeighbor(((cornerIndex & 2) | (cornerIndex & 4))>>1 );
        if(!temp || !temp->children){return NULL;}
        else{return & temp->children[pIndex];}
    }
    else{return NULL;}
}

template <class NodeData,class Real>
OctNode<NodeData,Real>* OctNode<NodeData,Real>::cornerNeighbor(const int& cornerIndex,const int& forceChildren){
    int pIndex,aIndex=0;
    if(!parent){return NULL;}

    pIndex=int(this-parent->children);
    aIndex=(cornerIndex ^ pIndex);	// The disagreement bits
    pIndex=(~pIndex)&7;				// The antipodal point
    if(aIndex==7){					// Agree on no bits
        return &parent->children[pIndex];
    }
    else if(aIndex==0){				// Agree on all bits
        OctNode* temp=((OctNode*)parent)->cornerNeighbor(cornerIndex,forceChildren);
        if(!temp){return NULL;}
        if(!temp->children){
            if(forceChildren){temp->initChildren();}
            else{return temp;}
        }
        return &temp->children[pIndex];
    }
    else if(aIndex==6){				// Agree on face 0
        OctNode* temp=((OctNode*)parent)->__faceNeighbor(0,cornerIndex & 1,0);
        if(!temp || !temp->children){return NULL;}
        else{return & temp->children[pIndex];}
    }
    else if(aIndex==5){				// Agree on face 1
        OctNode* temp=((OctNode*)parent)->__faceNeighbor(1,(cornerIndex & 2)>>1,0);
        if(!temp || !temp->children){return NULL;}
        else{return & temp->children[pIndex];}
    }
    else if(aIndex==3){				// Agree on face 2
        OctNode* temp=((OctNode*)parent)->__faceNeighbor(2,(cornerIndex & 4)>>2,0);
        if(!temp || !temp->children){return NULL;}
        else{return & temp->children[pIndex];}
    }
    else if(aIndex==4){				// Agree on edge 2
        OctNode* temp=((OctNode*)parent)->edgeNeighbor(8 | (cornerIndex & 1) | (cornerIndex & 2) );
        if(!temp || !temp->children){return NULL;}
        else{return & temp->children[pIndex];}
    }
    else if(aIndex==2){				// Agree on edge 1
        OctNode* temp=((OctNode*)parent)->edgeNeighbor(4 | (cornerIndex & 1) | ((cornerIndex & 4)>>1) );
        if(!temp || !temp->children){return NULL;}
        else{return & temp->children[pIndex];}
    }
    else if(aIndex==1){				// Agree on edge 0
        OctNode* temp=((OctNode*)parent)->edgeNeighbor(((cornerIndex & 2) | (cornerIndex & 4))>>1 );
        if(!temp || !temp->children){return NULL;}
        else{return & temp->children[pIndex];}
    }
    else{return NULL;}
}

////////////////
// VertexData //
////////////////

template<class NodeData,class Real>
long long OctNode<NodeData,Real>::CornerIndex(
    const NodeIndex& nIndex, const int& cIndex, const int& maxDepth)
{
    int idx[3];
    return CornerIndex(nIndex,cIndex,maxDepth,idx);
}

template<class NodeData,class Real>
long long OctNode<NodeData,Real>::CornerIndex(
    const NodeIndex& nIndex,const int& cIndex,const int& maxDepth,int idx[3]){
    int x[3];
    Cube::FactorCornerIndex(cIndex,x[0],x[1],x[2]);
    for(int i=0;i<3;i++)
        idx[i]=BinaryNode<Real>::CornerIndex(maxDepth+1,nIndex.depth,nIndex.offset[i],x[i]);

    // SIMON: Modified such that 20 octree levels are possible
    return (long long)(idx[0]) | (long long)(idx[1])<<21 | (long long)(idx[2])<<42;
}
