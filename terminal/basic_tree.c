#include "basic_tree.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 새로운 트리 노드 생성 함수
struct TreeNode *
createNode(const char *data, int numChildren)
{
    struct TreeNode *newNode = (struct TreeNode *)malloc(sizeof(struct TreeNode));
    strncpy(newNode->data, data, MAX_STRING_LENGTH - 1); // 문자열 복사 (null 종료)
    newNode->data[MAX_STRING_LENGTH - 1] = '\0';         // 문자열 끝에 null 추가
    newNode->numChildren = numChildren;
    newNode->children = (struct TreeNode **)malloc(numChildren * sizeof(struct TreeNode *));
    for (int i = 0; i < numChildren; i++)
    {
        newNode->children[i] = NULL;
    }
    return newNode;
}

// 트리에 자식 노드 추가 함수
void addChild(struct TreeNode *parent, struct TreeNode *child, int index)
{
    parent->children[index] = child;
}

// 트리 순회 함수 (전위 순회)
void preorderTraversal(struct TreeNode *root)
{
    if (root != NULL)
    {
        printf("%s ", root->data);
        for (int i = 0; i < root->numChildren; i++)
        {
            preorderTraversal(root->children[i]);
        }
    }
}