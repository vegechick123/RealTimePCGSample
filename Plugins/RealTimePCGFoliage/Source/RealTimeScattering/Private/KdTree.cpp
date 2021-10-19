// Fill out your copyright notice in the Description page of Project Settings.


#include "KdTree.h"


FKdTree* FKdTree::BuildKdTree(const TArray<FVector2D>& InData)
{
	FKdTree* result = new FKdTree();

	result->Data = &InData;

	TArray<int> Indices;
	for (int Index = 0; Index < result->Data->Num(); ++Index)
	{
		Indices.Add(Index);
	}
	int* data = Indices.GetData();
	result->Root = result->BuildNode(data, Indices.Num(), 0);
	return result;
}

float FKdTree::FindNearestPoint(FVector2D P)const
{

	float MinDist = 1e15f;
	if (Root != nullptr)
		FindNearstPointInNode(Root, P, MinDist);
	return MinDist;
}
void FKdTree::FindNearstPointInNode(FKdTreeNode* Current, FVector2D P, float& MinDist)const
{
	int Axis = Current->Axis;
	MinDist = FMath::Min(FVector2D::Distance((*Data)[Current->Index], P), MinDist);
	if (Current->ChildLeft)
	{
		FKdTreeNode* NextNode = nullptr;
		float CurrentAxisValue = (*Data)[Current->Index][Axis];
		if (P[Axis] <= CurrentAxisValue)
		{
			NextNode = Current->ChildRight;
			FindNearstPointInNode(Current->ChildLeft, P, MinDist);
		}
		else if (P[Axis] > CurrentAxisValue)
		{
			NextNode = Current->ChildLeft;
			if (Current->ChildRight != nullptr)
				FindNearstPointInNode(Current->ChildRight, P, MinDist);
		}
		if (NextNode != nullptr && FMath::Abs(CurrentAxisValue - P[Axis]) < MinDist)
		{
			FindNearstPointInNode(NextNode, P, MinDist);
		}
	}



}
void FKdTree::ClearKdtree()
{
	if (Root != nullptr)
		ClearNode(Root);
	Root = nullptr;
}

FKdTree::FKdTree()
{
}

FKdTree::~FKdTree()
{
	ClearKdtree();
}

FKdTreeNode* FKdTree::BuildNode(int* Indices, int NumData, int Depth)
{
	if (NumData <= 0)
	{
		return nullptr;
	}
	//Data;
	//TArray<FVector2D>*
	const int Axis = Depth % 2;
	const int Middle = NumData / 2;
	Sort(Indices, NumData, [this, Axis](const int& a, const int& b)
		{
			if ((*Data)[a][Axis] < (*Data)[b][Axis])
			{
				return true;
			}
			else
			{
				return false;
			}
		});
	//Indices->Sort();


	FKdTreeNode* NewNode = new FKdTreeNode();
	NewNode->Index = Indices[Middle];
	NewNode->Axis = Axis;
	NewNode->ChildLeft = BuildNode(Indices, Middle, Depth + 1);
	NewNode->ChildRight = BuildNode(Indices + Middle + 1, NumData - Middle - 1, Depth + 1);

	return NewNode;
}

void FKdTree::ClearNode(FKdTreeNode* Node)
{
	if (Node == nullptr)
	{
		return;
	}

	if (Node->ChildLeft)
	{
		ClearNode(Node->ChildLeft);
	}
	if (Node->ChildRight)
	{
		ClearNode(Node->ChildRight);
	}

	delete Node;
}
