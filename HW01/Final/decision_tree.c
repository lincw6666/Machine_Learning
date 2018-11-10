#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>

#define FEATURE_NUM		4
#define CLASS_NUM 		3
#define MAX_DEPTH		4
#define BUF_SIZE		255
#define TRAIN_DATA_SIZE	120
#define DATA_SIZE 		150


//********************************
//*        struct flower         *
//********************************
struct flower {
	double feature[FEATURE_NUM];	// [0]: sepal length, [1]: sepal width
									// [2]: petal length, [3]: petal width
	int class;			// 0: Setosa, 1: Versicolour, 2: Virginica
};


//********************************
//*         struct node          *
//********************************
struct node {
	struct node* parent;
	struct node* rightChild;
	struct node* leftChild;
	
	int feature;	// range: [0,3]
	double value;	// if >= value, go to rightChild
					// else, go to leftChild
	int class;
	bool isLeaf;
};

//********************************
//*      struct priority         *
//********************************
struct priority {
	int id;
	int value;
};


// global variable
int cmp_now = 0;
int depth = 0;


//********************************
//*          functions           *
//********************************
int cmp_flower(const void* a, const void* b);
int cmp_data(const void* a, const void* b);
struct flower get_data(struct flower data, char* str);
void build_tree(struct flower* start, struct flower* end,  struct node* nowNode); 
double get_rem(const double* class_num, const double* num);
void init_node(struct node* n, struct node* p);
void free_tree(struct node* n);
int test_data(struct node* n, struct flower* data);


//**********************************
//*              Main              *
//**********************************
int main(int argc, char** argv) 
{
	FILE* fp;
	char buf[BUF_SIZE];
	
	struct flower data[DATA_SIZE];
	struct flower flowers[TRAIN_DATA_SIZE];
	struct flower test[DATA_SIZE-TRAIN_DATA_SIZE];
	
	double accuracy = 0, precision[CLASS_NUM] = {0}, recall[CLASS_NUM] = {0};
	int TP[CLASS_NUM] = {0}, TN[CLASS_NUM] = {0}, FP[CLASS_NUM] = {0}, FN[CLASS_NUM] = {0};

	// open file 
	if (argc < 1) {
		fprintf(stderr, "Error!! No input file!!\n");
		exit(EXIT_FAILURE);
	}
	else if (!(fp = fopen(argv[1], "r"))) { 
		fprintf(stderr, "Error!! File not found!!\n");
		exit(EXIT_FAILURE);
	}
	
	//**********************************
	//*          Data Shuffle          *
	//**********************************
	struct priority *prioritys = malloc(sizeof(struct priority)*DATA_SIZE);
	struct flower *tmp_data = malloc(sizeof(struct flower)*DATA_SIZE);
	// get data, set priority
	srand(time(NULL));
	fseek(fp, 0, SEEK_SET);
	for (int i = 0; i < DATA_SIZE; i++) {
		char* ch = fgets(buf, BUF_SIZE, fp);
		tmp_data[i] = get_data(tmp_data[i], buf);
		prioritys[i].id = i;
		prioritys[i].value = rand()%1000000;
	}
	// sort data by priority, if the value of priority[i] is smaller, then data[i] has higher priority
	qsort(prioritys, DATA_SIZE, sizeof(struct priority), cmp_data);
	// put data to the right place according to the priority
	for (int i = 0; i < DATA_SIZE; i++) {
		for (int j = 0; j < FEATURE_NUM; j++)
			data[i].feature[j] = tmp_data[prioritys[i].id].feature[j];
		data[i].class = tmp_data[prioritys[i].id].class;
	}
	free(prioritys);
	free(tmp_data);
	prioritys = NULL;
	tmp_data = NULL;
	//**********************************
	//*        End Data Shuffle        *
	//**********************************
	

	//**********************************
	//*          Train Data            *
	//**********************************
	for (int test_data_start = 0; test_data_start < DATA_SIZE; test_data_start += 30) {
		struct node* root = malloc(sizeof(struct node));
		int tmp_accu = 0;
		
		// initialize root 
		init_node(root, NULL);

		// get trainning data
		for (int i = 0, now = 0; i < DATA_SIZE; i++) {
			if (i < test_data_start || i >= test_data_start+30) {
				for (int j = 0; j < FEATURE_NUM; j++)
					flowers[now].feature[j] = data[i].feature[j];
				flowers[now].class = data[i].class;
				now++;
			}
			else {
				for (int j = 0; j < FEATURE_NUM; j++)
					test[i-test_data_start].feature[j] = data[i].feature[j];
				test[i-test_data_start].class = data[i].class;
			}
		}
		
		// build decision tree
		depth = 0;
		build_tree(flowers, flowers+TRAIN_DATA_SIZE-1, root);

		// put data into the decision tree
		for (int i = 0; i < DATA_SIZE-TRAIN_DATA_SIZE; i++) {
			int get_class = test_data(root, &test[i]);
			
			if (get_class == test[i].class) {
				tmp_accu++;
				for (int j = 0; j < CLASS_NUM; j++) {
					if (j == get_class) TP[j]++;
					else TN[j]++;
				}
			}
			else {
				for (int j = 0; j < CLASS_NUM; j++) {
					if (j == get_class) FP[j]++;
					else if (j == test[i].class) FN[j]++;
					else TN[j]++;
				}
			}
		}
		accuracy += (double) tmp_accu / (double) (30.0);

		// free decision tree
		free_tree(root);
	}
	//**********************************
	//*        End Train Data          *
	//**********************************


	// calculate answer
	printf("%.7lf\n", accuracy/(double)5.0);
	for (int i = 0; i < CLASS_NUM; i++)
		printf("%.7lf %.7lf\n", (double)TP[i]/(double)(TP[i]+FP[i]), (double)TP[i]/(double)(TP[i]+FN[i]));

	fclose(fp);
	return 0;

}
//**********************************
//*          End Main              *
//**********************************


// func: cmp_flower. compare flower, for qsort 
int cmp_flower(const void* a, const void* b) {
	double c = (*(struct flower*)a).feature[cmp_now];
	double d = (*(struct flower*)b).feature[cmp_now];
	int e = (*(struct flower*)a).class;
	int f = (*(struct flower*)b).class;

	if (c < d) return -1;
	else if (c == d) return 0; 
	else return 1;
}

// func: cmp_data. compare data priority, for qsort
int cmp_data(const void* a, const void* b) {
	int c = (*(struct priority*)a).value;
	int d = (*(struct priority*)b).value;
	return c < d;
}

// func: get_data
struct flower get_data(struct flower data, char* str) {
	int now = 0;

	// get feature
	for (int i = 0; i < FEATURE_NUM; i++) {
		double temp = 0.0;
		double base = 0.1;
		while (str[now++] != '.')
			temp = temp*10 + (str[now-1]&0xF);
		while (str[now++] != ',') {
			temp += (str[now-1]&0xF) * base;
			base /= 10;
		}
		data.feature[i] = temp;
	}
	// get class
	while (str[now++] != '-')
		;
	if (str[now] == 's') data.class = 0;
	else if (str[now+1] == 'e') data.class = 1;
	else data.class = 2; 
	
	return data;	
}

// func: build_tree
void build_tree(struct flower* start, struct flower* end,  struct node* nowNode) {
	struct flower *temp;	
	double min_rem = -1;
	int class_num[CLASS_NUM] = {0};
	int split = -1;

	// count each class has how many instance
	for (temp = start; temp <= end; temp++) 
		class_num[temp->class]++;
	
	// there are four feature, we need to choose the smallest rem to build the decision tree
	for (int now_f = 0; now_f < FEATURE_NUM; now_f++) {		// now_f: now feature, range [0, 3]
		int now_num[CLASS_NUM] = {0};		// numbers of each class (accumulate)

		// sort data
		cmp_now = now_f;
		qsort(start, end-start+1, sizeof(struct flower), cmp_flower);

		now_num[start->class]++;
		temp = start+1;
		while (temp <= end) {
			if (temp->feature[now_f] != (temp-1)->feature[now_f] && temp->class != (temp-1)->class) {
				//************************************************************************************************ 
				//* feature:...|6.4 |6.4 |6.5 |6.5 |...|6.5 |6.6 |...
				//* class:  ...| 0  | 0  | 1  | 2  |...| 1  | 2  |...
				//* data :	...|data|data|data|data|...|data|data|...
				//*                  ----|<---------------->|
				//*                    ^        tmp_num
				//*                    |
				//*                 tmp_pre
				//************************************************************************************************
				struct flower *tmp_pre = temp-1;	// if it is a split point, pre = tmp_pre
				int tmp_num[CLASS_NUM] = {0};		// numbers of each class in the range above
				int max_num = 0, tmp_f;				// we classified the group with same feature to class "tmp_f"

				tmp_num[temp->class]++;
				temp++;
				while (temp <= end && temp->feature[now_f] == (temp-1)->feature[now_f]) {
					tmp_num[temp->class]++;
					temp++;
				}
				
				// classify the group, choose the bigest class to be the class of the group
				for (int i = 0; i < CLASS_NUM; i++)
					if (tmp_num[i] > max_num) max_num = tmp_num[i], tmp_f = i;

				if (tmp_f != tmp_pre->class) {
					const double tmp_rem = get_rem((double*)now_num, (double*)class_num);	// for instance less than the spilt point

					if (min_rem = -1 || tmp_rem < min_rem) {
						min_rem = tmp_rem;
						nowNode->feature = now_f;
						nowNode->value = (double) (tmp_pre->feature[now_f]+(tmp_pre+1)->feature[now_f])/2.0;
						split = tmp_pre-start;
						max_num = 0;
						for (int i = 0; i < CLASS_NUM; i++)
							if (now_num[i] > max_num) max_num = now_num[i], tmp_f = i;
						nowNode->class = tmp_f;
					}
				}
				now_num[0] += tmp_num[0], now_num[1] += tmp_num[1], now_num[2] += tmp_num[2];
			}
			else now_num[temp->class]++, temp++;
		}
	}

	if (min_rem != -1 && depth < MAX_DEPTH) {
		// build childs
		nowNode->rightChild = malloc(sizeof(struct node));
		nowNode->leftChild = malloc(sizeof(struct node));
		// initialize childs
		init_node(nowNode->rightChild, nowNode);
		init_node(nowNode->leftChild, nowNode);
		cmp_now = nowNode->feature;
		qsort(start, end-start+1, sizeof(struct flower), cmp_flower);
		build_tree(start, start+split, nowNode->leftChild);
		build_tree(start+split+1, end, nowNode->rightChild);
		depth++;
	}
	else {		// nowNode is a leaf
		int max_num = 0;
		
		nowNode->isLeaf = true;
		for (int i = 0; i < CLASS_NUM; i++) 
			if (class_num[i] > max_num) max_num = class_num[i], nowNode->class = i;
	}
}

// func: get_rem
double get_rem(const double* class_num, const double* num) {
	const double total_lt = class_num[0] + class_num[1] + class_num[2];	// numbers of instance less than split point
	const double total = num[0] + num[1] + num[2];
	const double p1 = class_num[0] / total_lt;
	const double p2 = class_num[1] / total_lt;	
	const double p3 = class_num[2] / total_lt;
	const double p4 = (num[0]-class_num[0]) / (total-total_lt);
	const double p5 = (num[1]-class_num[1]) / (total-total_lt);
	const double p6 = (num[2]-class_num[2]) / (total-total_lt);
	return total_lt*(-1*(p1*log2(p1)+p2*log2(p2)+p3*log2(p3)))/total + (total-total_lt)*(-1*(p4*log2(p4)+p5*log2(p5)+p6*log2(p6)))/total;
}

// func: init_node
void init_node(struct node* n, struct node* p) {
	n->parent = p, n->rightChild = NULL, n->leftChild = NULL;
	n->feature = 0.0, n->value = 0.0, n->class = 0, n->isLeaf = false;
}

// func: free_tree
void free_tree(struct node* n) {
	if (!n->rightChild && !n->leftChild) {
		n->parent = NULL;
		free(n);
		n = NULL;
	}
	else {
		if (n->leftChild != NULL) free_tree(n->leftChild);
		if (n->rightChild != NULL) free_tree(n->rightChild);
		n->parent = NULL;
		free(n);
		n = NULL;
	}
}

// func: test_data
int test_data(struct node* n, struct flower* data) {
	if (n->isLeaf)
		return n->class;
	else if (data->feature[n->feature] < n->value)
		return (!n->leftChild)? n->class : test_data(n->leftChild, data);
	else
		return (!n->rightChild)? n->class : test_data(n->rightChild, data);
}

