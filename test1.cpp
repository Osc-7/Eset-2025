#include "include/Eset.hpp"
#include <algorithm>
#include <chrono>
#include <climits>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <set>
#include <vector>

using namespace std;
using namespace chrono;

// 测试配置
const vector<size_t> TEST_SIZES = {10'000, 100'000, 1'000'000};
const int RANDOM_SEED = 42;
const int WARMUP_RUNS = 2;
const int TEST_RUNS = 5;

// 生成测试数据
vector<int> generate_test_data(size_t n, const string &type) {
  vector<int> data(n);
  mt19937 rng(RANDOM_SEED);

  if (type == "random") {
    uniform_int_distribution<int> dist(1, INT_MAX);
    generate(data.begin(), data.end(), [&]() { return dist(rng); });
  } else if (type == "sorted") {
    iota(data.begin(), data.end(), 1); // 1, 2, 3...
  } else if (type == "reverse") {
    iota(data.rbegin(), data.rend(), 1); // n, n-1,...1
  } else if (type == "duplicate") {
    uniform_int_distribution<int> dist(1, 100); // 大量重复
    generate(data.begin(), data.end(), [&]() { return dist(rng); });
  }

  return data;
}

// 精确计时模板
template <typename Func> auto measure_time(Func &&func, int runs = 1) {
  for (int i = 0; i < WARMUP_RUNS; ++i)
    func();

  nanoseconds total{0};
  for (int i = 0; i < runs; ++i) {
    auto start = high_resolution_clock::now();
    func();
    auto end = high_resolution_clock::now();
    total += duration_cast<nanoseconds>(end - start);
  }
  return total / runs;
}

// 基本性能测试
template <typename SetType>
tuple<nanoseconds, nanoseconds, nanoseconds>
benchmark_basic_operations(const vector<int> &data) {
  SetType s;

  auto insert_time = measure_time(
      [&] {
        for (const auto &x : data)
          s.emplace(x);
      },
      TEST_RUNS);

  auto find_time = measure_time(
      [&] {
        for (const auto &x : data)
          s.find(x);
      },
      TEST_RUNS);

  auto erase_time = measure_time(
      [&] {
        for (const auto &x : data)
          s.erase(x);
      },
      TEST_RUNS);

  return make_tuple(insert_time, find_time, erase_time);
}

// 范围查询测试
template <typename SetType>
nanoseconds benchmark_range_query(const vector<int> &data) {
  SetType s;
  for (const auto &x : data)
    s.emplace(x);

  if constexpr (requires { s.range(0, 0); }) {
    auto time = measure_time(
        [&] { volatile size_t cnt = s.range(data.front(), data.back()); },
        TEST_RUNS);
    return time;
  }
  return nanoseconds(0);
}

// 对比两种结构的性能
void compare_benchmarks(const vector<int> &data, const string &data_type) {
  cout << "\n=== Comparing std::set and ESet (" << data.size() << " elements, "
       << data_type << " data) ===\n";

  // 测试 std::set
  auto [std_insert, std_find, std_erase] =
      benchmark_basic_operations<set<int>>(data);
  auto std_range = benchmark_range_query<set<int>>(data);

  // 测试 ESet
  auto [eset_insert, eset_find, eset_erase] =
      benchmark_basic_operations<ESet<int>>(data);
  auto eset_range = benchmark_range_query<ESet<int>>(data);

  // 输出对比表格
  cout << setw(15) << "Operation" << setw(15) << "std::set" << setw(15)
       << "ESet" << setw(15) << "Difference (%)" << endl;
  cout << string(60, '-') << "\n";

  auto print_result = [](const string &name, nanoseconds std_time,
                         nanoseconds eset_time) {
    double diff = (double(eset_time.count()) / std_time.count() - 1) * 100;
    cout << setw(15) << name << setw(15) << std_time.count() << setw(15)
         << eset_time.count() << setw(14) << fixed << setprecision(2) << diff
         << " %\n";
  };

  print_result("Insert", std_insert, eset_insert);
  print_result("Find", std_find, eset_find);
  print_result("Erase", std_erase, eset_erase);

  if (std_range.count() > 0) {
    print_result("Range Query", std_range, eset_range);
  } else {
    cout << setw(15) << "Range Query" << setw(15) << "N/A" << setw(15)
         << eset_range.count() << " ns\n";
  }
}

int main() {
  ofstream out("results_O2.txt");
  cout.rdbuf(out.rdbuf());

  cout << "===== Enhanced ESet vs std::set Benchmark =====\n";
  cout << "Config: Warmup=" << WARMUP_RUNS << ", TestRuns=" << TEST_RUNS
       << "\n\n";

  for (size_t size : TEST_SIZES) {
    cout << "\n■■■■■ TEST SIZE: " << size << " ■■■■■\n";

    // 随机数据测试
    auto random_data = generate_test_data(size, "random");
    compare_benchmarks(random_data, "random");

    // 有序数据测试
    auto sorted_data = generate_test_data(size, "sorted");
    compare_benchmarks(sorted_data, "sorted");

    // 重复数据测试
    if (size <= 100'000) { // 避免太大测试集
      auto dup_data = generate_test_data(size, "duplicate");
      compare_benchmarks(dup_data, "duplicate");
    }
  }

  cout.rdbuf(nullptr);
  return 0;
}
