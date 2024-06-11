#ifndef BASIC_TREE_H
#define BASIC_TREE_H

#define MAX_STRING_LENGTH 32

// 트리 노드 구조체 정의
struct TreeNode
{
    char data[MAX_STRING_LENGTH]; // 노드의 문자열 데이터
    struct TreeNode **children;   // 자식 노드 배열을 가리키는 포인터
    int numChildren;              // 자식 노드의 개수
};

// 새로운 트리 노드 생성 함수
struct TreeNode *createNode(const char *data, int numChildren);

// 트리에 자식 노드 추가 함수
void addChild(struct TreeNode *parent, struct TreeNode *child, int index);

// 트리 순회 함수 (전위 순회)
void preorderTraversal(struct TreeNode *root);

#endif /* BASIC_TREE_H */