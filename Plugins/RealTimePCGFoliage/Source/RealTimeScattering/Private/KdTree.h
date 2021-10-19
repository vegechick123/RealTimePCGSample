// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */
struct FKdTreeNode
{
    int Index = -1;
    int Axis = -1;
    FKdTreeNode* ChildLeft = nullptr;
    FKdTreeNode* ChildRight = nullptr;
};

class FKdTree
{
public:
    FKdTreeNode* Root;
    const TArray<FVector2D>* Data;
public:

    static FKdTree* BuildKdTree(const TArray<FVector2D>& Data);
    float FindNearestPoint(FVector2D p)const;
    void FindNearstPointInNode(FKdTreeNode* current, FVector2D p, float &MinDist)const;
    void ClearKdtree();
    void ValidateKdtree();
    void DumpKdTree();
    FKdTree();
	~FKdTree();
private:
    FKdTreeNode* BuildNode(int* Indices, int NumData, int Depth);
    void ClearNode(FKdTreeNode* Node);
};
