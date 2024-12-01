// #include <iostream>
// #include <unordered_set>
// #include <vector>

// class Object
// {
// public:
//     bool marked;                     // 标记状态
//     std::vector<Object*> references; // 引用的对象

//     Object() : marked(false) {}

//     void addReference(Object* obj) { references.push_back(obj); }
// };

// // Mark 阶段
// void mark(Object* obj)
// {
//     if (obj == nullptr || obj->marked) { return; }
//     obj->marked = true; // 标记为可达
//     for (Object* ref : obj->references)
//     {
//         mark(ref); // 递归标记引用的对象
//     }
// }

// // Sweep 阶段
// void sweep(std::vector<Object*>& heap)
// {
//     for (auto it = heap.begin(); it != heap.end();)
//     {
//         if (!(*it)->marked)
//         {
//             delete *it;          // 释放内存
//             it = heap.erase(it); // 从堆中移除对象
//         }
//         else
//         {
//             (*it)->marked = false; // 重置标记状态
//             ++it;
//         }
//     }
// }

// int main()
// {
//     std::vector<Object*> heap; // 模拟堆内存

//     // 创建一些对象
//     Object* obj1 = new Object();
//     Object* obj2 = new Object();
//     Object* obj3 = new Object();
//     Object* obj4 = new Object();

//     // 建立引用关系
//     obj1->addReference(obj2);
//     obj2->addReference(obj3);
//     // obj3->addReference(obj4); // obj1 -> obj2 -> obj3 -> obj4

//     // 假设 obj1 是根对象
//     heap.push_back(obj1);
//     heap.push_back(obj2);
//     heap.push_back(obj3);
//     heap.push_back(obj4);

//     // 执行 Mark 和 Sweep
//     mark(obj1);  // 从根对象开始标记

//     sweep(heap); // 执行清扫

//     // 输出结果
//     std::cout << "Remaining objects in heap: " << heap.size() << std::endl;

//     // 清理剩余对象
//     for (Object* obj : heap) { delete obj; }

//     return 0;
// }

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define FILE_SIZE   (100 * 1024 * 1024) // 100 MB
#define BUFFER_SIZE 4096

void write_test_fwrite(FILE* fp)
{
    char buffer[BUFFER_SIZE];
    memset(buffer, 'A', BUFFER_SIZE);

    for (int i = 0; i < FILE_SIZE / BUFFER_SIZE; i++) { fwrite(buffer, 1, BUFFER_SIZE, fp); }
}

void write_test_fputc(FILE* fp)
{
    for (int i = 0; i < FILE_SIZE; i++) { fputc('A', fp); }
}

void read_test_fread(FILE* fp)
{
    char buffer[BUFFER_SIZE];
    while (fread(buffer, 1, BUFFER_SIZE, fp) > 0)
    {
        // 读取但不做任何处理
    }
}

void read_test_fgetc(FILE* fp)
{
    while (fgetc(fp) != EOF)
    {
        // 读取但不做任何处理
    }
}

double time_diff(struct timespec start, struct timespec end)
{
    return (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
}

int main()
{
    FILE* fp;
    struct timespec start, end;
    double elapsed;

    // 写入测试
    fp = fopen("test_file.bin", "wb");
    if (fp == NULL)
    {
        perror("Error opening file for writing");
        return 1;
    }

    clock_gettime(CLOCK_MONOTONIC, &start);
    write_test_fwrite(fp);
    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed = time_diff(start, end);
    printf("fwrite time: %.6f seconds\n", elapsed);

    fclose(fp);

    fp = fopen("test_file.bin", "wb");
    if (fp == NULL)
    {
        perror("Error opening file for writing");
        return 1;
    }

    clock_gettime(CLOCK_MONOTONIC, &start);
    write_test_fputc(fp);
    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed = time_diff(start, end);
    printf("fputc time: %.6f seconds\n", elapsed);

    fclose(fp);

    // 读取测试
    fp = fopen("test_file.bin", "rb");
    if (fp == NULL)
    {
        perror("Error opening file for reading");
        return 1;
    }

    clock_gettime(CLOCK_MONOTONIC, &start);
    read_test_fread(fp);
    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed = time_diff(start, end);
    printf("fread time: %.6f seconds\n", elapsed);

    fseek(fp, 0, SEEK_SET); // 重置文件指针到开始

    clock_gettime(CLOCK_MONOTONIC, &start);
    read_test_fgetc(fp);
    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed = time_diff(start, end);
    printf("fgetc time: %.6f seconds\n", elapsed);

    fclose(fp);

    remove("test_file.bin"); // 删除测试文件

    return 0;
}
