## 不使用 stack

- dfs / 前序 都是采用相同的模式 : 从 stack 的 top 取元素，然后 push 进去其 neighbor
- 但是中序遍历不同，就是需要左边完全 push, 然后 pop 出来就是
```cpp
class Solution {
public:
  vector<int> inorderTraversal(TreeNode *root) {
    vector<TreeNode *> stk;
    vector<int> res;
    while(root != NULL  || !stk.empty()){
      while(root != NULL){
        stk.push_back(root);
        root = root->left;
      }

      TreeNode * n = stk.back();
      stk.pop_back();
      res.push_back(n->val);
      root = n->right;
    }

    return res;
  }
};
```
- 后序其实就是前序的内容反过来

## 最近公共祖先
没有什么高级实现，就是做深度遍历，谁最开始发现两个 node，谁就是 lowest-common-ancestor

https://leetcode-cn.com/problems/lowest-common-ancestor-of-a-binary-tree/comments/

```c
class Solution {
public:
    TreeNode* lowestCommonAncestor(TreeNode* root, TreeNode* p, TreeNode* q) {
        if (!root || root == p || root == q) return root;

        TreeNode* left = lowestCommonAncestor(root->left, p, q);
        TreeNode* right = lowestCommonAncestor(root->right, p, q);

        if (left && right) return root;   // p、q 分居两侧，root 就是 LCA
        return left ? left : right;       // 否则把找到的那一边往上传递
    }
};
```
这里实现有一个小小的技巧，如果 LCA 找到了，那么可以直接向上传递。

## 红黑树

<script src="https://giscus.app/client.js"
        data-repo="martins3/martins3.github.io"
        data-repo-id="MDEwOlJlcG9zaXRvcnkyOTc4MjA0MDg="
        data-category="Show and tell"
        data-category-id="MDE4OkRpc2N1c3Npb25DYXRlZ29yeTMyMDMzNjY4"
        data-mapping="pathname"
        data-reactions-enabled="1"
        data-emit-metadata="0"
        data-theme="light"
        data-lang="zh-CN"
        crossorigin="anonymous"
        async>
</script>

本站所有文章转发 **CSDN** 将按侵权追究法律责任，其它情况随意。
