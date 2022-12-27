#include <iostream>
#include <functional>

class Tree {
	public:
		typedef struct node_t {
			int           id, n;
			struct node_t *even, *odd;
			std::string get_name() {
				return "node" + std::to_string(id);
			}
		} node_t;
		typedef enum {
			pre_order,
			in_order,
			post_order
		} traversal_order_t;
		typedef std::function<void(node_t*, int)> fn_t;
	public:
		Tree() : root(nullptr), max_id(0) {}
		~Tree();
		void gen_tree(int n, int max_depth);
		void iterate(fn_t fn, traversal_order_t order);
	private:
		void gen_tree_hlp(node_t **node, int n, int max_depth);
		void iterate_hlp(node_t *node, fn_t fn, traversal_order_t order, int depth);
		int  get_new_id();

		node_t *root;
		int    max_id;
};

int Tree::get_new_id() {
	return max_id++;
}

void Tree::iterate_hlp(node_t *node, fn_t fn, traversal_order_t order, int depth) {
	if (node == nullptr)
		return;
	if (order == pre_order)
		fn(node, depth);
	iterate_hlp(node->even, fn, order, depth + 1);
	if (order == in_order)
		fn(node, depth);
	iterate_hlp(node->odd, fn, order, depth + 1);
	if (order == post_order)
		fn(node, depth);
}

void Tree::iterate(fn_t fn, traversal_order_t order) {
	iterate_hlp(root, fn, order, 0);
}

Tree::~Tree() {
	iterate([](node_t *node, int depth){ delete node; }, post_order);
	root = nullptr;
}

void Tree::gen_tree(int n, int max_depth) {
	gen_tree_hlp(&root, n, max_depth);
}

void Tree::gen_tree_hlp(node_t **node, int n, int max_depth) {
	if ((max_depth <= 0) || (n <= 0)) {
		*node = nullptr;
		return;
	}
	*node       = new node_t;
	(*node)->n  = n;
	(*node)->id = get_new_id();
	gen_tree_hlp(&(*node)->even, 2 * n, max_depth - 1);
	if ((n - 1) % 3 == 0)
		gen_tree_hlp(&(*node)->odd, (n - 1) / 3, max_depth + 1);
}

int main() {
	Tree t;
	t.gen_tree(1, 10);
/*	auto tabs = [](int t){ std::string s; while (t-- > 0) s += "  "; return s; };
	t.iterate([&tabs](Tree::node_t *node, int depth) {
		std::cout << tabs(depth) << node->n << std::endl;
	}, Tree::traversal_order_t::pre_order);
*/
	std::cout << "digraph collatz_tree {" << std::endl;
	auto labels = [](Tree::node_t *node, int depth) {
		auto id = std::to_string(node->id);
		std::cout << "\t" << node->get_name() + " [label=\"" << node->n << "\"]" << std::endl;
	};
	t.iterate(labels, Tree::traversal_order_t::pre_order);

	auto edges = [](Tree::node_t *node, int depth) {
		if (node->even != nullptr)
			std::cout << "\t" << node->get_name() + " -> " + node->even->get_name() + ";" << std::endl;
		if (node->odd != nullptr)
			std::cout << "\t" << node->get_name() + " -> " + node->odd->get_name() + ";" << std::endl;
	};
	t.iterate(edges, Tree::traversal_order_t::pre_order);
	std::cout << "}" << std::endl;
}
