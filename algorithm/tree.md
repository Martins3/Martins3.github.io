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
