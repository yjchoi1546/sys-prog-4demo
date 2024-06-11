#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "basic_tree.h"

void showChildNodes(struct TreeNode *node, struct TreeNode *root);
void goToParent(struct TreeNode *node, struct TreeNode *root);

// 상위 노드로 이동하는 함수
void goToParent(struct TreeNode *node, struct TreeNode *root)
{
    if (node == root)
    {
        printf("더 이상 상위 노드가 없습니다.\n");
    }
    else
    {
        printf("상위 노드로 이동합니다.\n");
        showChildNodes(root, root);
    }
}

// 자식 노드를 보여주고 입력된 순번에 따라 해당 순번의 문자열을 출력하고 그 자식 노드를 보여주는 함수
void showChildNodes(struct TreeNode *node, struct TreeNode *root)
{
    if (node == NULL || node->numChildren == 0)
    {
        printf("자식 노드가 없습니다.\n");
        return;
    }

    // 자식 노드 출력
    printf("자식 노드:\n");
    for (int i = 0; i < node->numChildren; i++)
    {
        printf("%d: %s\n", i + 1, node->children[i]->data);
    }
    printf("0: 상위 노드로 이동\n");

    // 사용자 입력 받기
    int choice;
    printf("선택할 자식 노드의 번호를 입력하세요: ");
    scanf("%d", &choice);
    fflush(stdin); // 입력 버퍼 비우기

    // 입력된 순번에 따라 동작 수행
    if (choice >= 1 && choice <= node->numChildren)
    {
        struct TreeNode *selectedNode = node->children[choice - 1];
        printf("선택된 자식 노드: %s\n", selectedNode->data);
        showChildNodes(selectedNode, root); // 선택된 자식 노드의 자식 노드 보여주기
    }
    else if (choice == 0)
    {
        goToParent(node, root); // 상위 노드로 이동
    }
    else
    {
        printf("올바른 번호를 입력하세요.\n");
    }
}

int main()
{
    // 트리 노드 생성 (이전 예제와 동일)
    struct TreeNode *root = createNode("Root", 2);
    struct TreeNode *adminNode = createNode("Admin", 3);
    struct TreeNode *mileageNode = createNode("Mileage Usage", 1);
    struct TreeNode *userListNode = createNode("User List", 2);
    struct TreeNode *userRecordNode = createNode("User Record", 0);
    struct TreeNode *adminManageNode = createNode("Admin Account Management", 2);
    struct TreeNode *addAdminNode = createNode("Add Admin Account", 1);
    struct TreeNode *deleteAdminNode = createNode("Delete Admin Account", 1);
    struct TreeNode *generalNode = createNode("General", 1);

    // 트리에 자식 노드 추가 (이전 예제와 동일)
    addChild(root, adminNode, 0);
    addChild(root, generalNode, 1);

    addChild(adminNode, mileageNode, 0);
    addChild(adminNode, userListNode, 1);
    addChild(adminNode, adminManageNode, 2);

    addChild(userListNode, userRecordNode, 0);
    addChild(userListNode, deleteAdminNode, 1);

    addChild(adminManageNode, addAdminNode, 0);
    addChild(adminManageNode, deleteAdminNode, 1);

    // 루트 노드에서 시작하여 자식 노드를 보여주고 사용자 입력을 처리
    showChildNodes(root, root);

    return 0;
}