#include <benchmark/benchmark.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <memory>
#include <unordered_map>

#include "jsonL.hpp"

static std::string get_json(const std::string& file) {
  std::ifstream ifs(file);
  std::stringstream ss;
  ss << ifs.rdbuf();
  return ss.str();
}

template <class Json>
static void BM_Parse(benchmark::State& state, std::string filename, std::string data) {
  std::string err;
  for (auto _ : state) {
    Json::parse(data, err);
  }
  if (!err.empty()) {
    std::cout << err << std::endl;
    return;
  } 

  state.SetLabel(filename);
  state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(data.size()));
}

template <typename Json>
static void BM_Dump(benchmark::State &state, std::string filename,
                      std::string data) {
  std::string err;
  auto json = Json::parse(data, err);
  if (!err.empty()) {
    std::cout << err << std::endl;
    return;
  } 
  
  for (auto _ : state) {
      json.dump();
  }

  state.SetLabel(filename);
  state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(data.size()));
}

struct Item{
  std::string filename;
  std::string path;
  bool existed = {false};
  std::string json;
};

int main(int argc, char **argv) {
  std::vector<std::string> files = {"book", "canada", "citm_catalog", "fgo",
                                    "github_events", "gsoc-2018", "lottie", "otfcc", "poet", "twitter", "twitterscaped"};
  // , "gsoc-2019", "lottie", "otfcc", "poet", "twitter", "twitterscaped"

  //1. 初始化Items
  std::unordered_map<std::string, std::unique_ptr<Item>> items;

  for (auto& file : files) {
    auto item_ptr(std::make_unique<Item>());
    item_ptr->filename = file + ".json";
    item_ptr->path = "../testdata/" + file + ".json";

    std::ifstream ifs(item_ptr->path);
    if (!ifs.is_open()) {
      std::cout << "error : open file!" << std::endl;
      break;
    }
    item_ptr->existed = true;
    
    std::stringstream ss;
    ss << ifs.rdbuf();
    item_ptr->json = std::move(ss.str());
    items[file] = std::move(item_ptr);
  }
  auto filename = items["fgo"]->filename;
  auto data = items["fgo"]->json; 

#define REGBM(ACT, JSON, FNAME)             \
  do {                                      \
      benchmark::RegisterBenchmark(         \
        ("BM_" #ACT "-" #JSON "-" #FNAME),  \
        BM_##ACT<JSON::Json>,               \
        items[#FNAME]->filename,            \
        items[#FNAME]->json);               \
  } while (0)

#define CMPJSON(ACT, FNAME)                 \
  do {                                      \
      REGBM(ACT, jsonL,FNAME);              \
  } while (0)

#define CMP(FNAME)                          \
  do {                                      \
    CMPJSON(Parse, FNAME);                  \
    CMPJSON(Dump, FNAME);                   \
  } while (0)

   CMP(book);
   CMP(canada);
  // CMP(citm_catalog);
  // CMP(fgo);
  // CMP(github_events);
  // CMP(gsoc-2018);
  // CMP(lottie);
  // CMP(otfcc);
  // CMP(poet);
  // CMP(twitter);
  // CMP(twitterscaped);

  benchmark::Initialize(&argc, argv);
  benchmark::RunSpecifiedBenchmarks();
  benchmark::Shutdown();
  return 0;
}


