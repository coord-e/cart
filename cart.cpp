#include <iostream>
#include <numeric>
#include <algorithm>
#include <sstream>

#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <args.hxx>

#include <clang-c/Index.h>

// function 'tokenize' is based on tokenizer.cc (http://tlab.hatenablog.com/entry/2013/01/05/142823)
// Copyright (c) 2013 Shinya
// The MIT License (MIT) http://opensource.org/licenses/mit-license.php
template<typename OutputIterator>
OutputIterator tokenize(OutputIterator first, const std::string& filepath) {
  auto const index = clang_createIndex(1, 0); // exclude_decls_from_pch:1, display_diagnostics:0
  clang_CXIndex_setGlobalOptions(index, CXGlobalOpt_None);
  const char* args[] = { "-Xclang", "-cc1" };
  auto unit = clang_createTranslationUnitFromSourceFile(index, filepath.c_str(),
          sizeof(args) / sizeof(char*), args,
          0, nullptr);
  if (unit != NULL) {
    auto const cursor = clang_getTranslationUnitCursor(unit);
    auto const range = clang_getCursorExtent(cursor);
    CXToken* tokens = nullptr;
    unsigned int num_tokens = 0;
    clang_tokenize(unit, range, &tokens, &num_tokens);

    std::pair<std::string, int> macro_holder = {"", -1};
    for (int i = 0; i < num_tokens; ++i) {
      auto const& token = tokens[i];
      auto const spelling = clang_getTokenSpelling(unit, token);
      CXFile file;
      unsigned line, column, offset;
      clang_getSpellingLocation(clang_getTokenLocation(unit, token), &file, &line, &column, &offset);
      std::string thes = clang_getCString(spelling);
      if(*std::cbegin(thes) == '"' && *(std::cend(thes) - 1) == '"' && thes.length() != 2 && macro_holder.second == -1){
        auto it = std::cbegin(thes) + 1;
        auto end = std::cend(thes) - 1;
        for(; it != end; it++){
          if(*it == '\\'){
            it++;
            *first++ = std::string("\"\\") + *it + "\"";
          }else
            *first++ = std::string("\"") + *it + "\"";
        }
      }else if (*std::cbegin(thes) == '#' && macro_holder.second == -1) { //start of macros
        //std::cout << "SOM: " << thes << std::endl;
        macro_holder.first = "#";
        macro_holder.second = line;
      }else if (*std::cbegin(thes) == '#' && macro_holder.second != -1 && macro_holder.second != line) { //end & start of macros
        //std::cout << "SEOM: " << thes << std::endl;
        *first++ = macro_holder.first+'\n';
        macro_holder.first = "#";
        macro_holder.second = line;
      }else{
        if(macro_holder.second != -1){
          if(macro_holder.second == line){ // in macros
            //std::cout << "IOM: " << thes << std::endl;
            macro_holder.first += thes;
            thes = "";
          }else{ // end of macros
            //std::cout << "EOM: " << macro_holder.first << std::endl;
            *first++ = macro_holder.first;
            macro_holder.first = "";
            macro_holder.second = -1;
          }
        }
        auto const kind = clang_getTokenKind(token);
        if(kind != CXToken_Comment){
          *first++ = kind != CXToken_Punctuation ? thes + " " : thes;
        }
      }
      clang_disposeString(spelling);
    }
    clang_disposeTokens(unit, tokens, num_tokens);
    clang_disposeTranslationUnit(unit);
  } else {
    std::cerr << "Failed to tokenize: \"" << filepath << "\"" << std::endl;
  }
  clang_disposeIndex(index);
  return first;
}

int main (int argc, char **argv)
{
  args::ArgumentParser argparser("cart: Convert a c/c++ code to ascii art");
  args::HelpFlag help(argparser, "help", "Print this help", {'h', "help"});
  args::Positional<std::string> arg_source(argparser, "source", "Path to c source file");
  args::Positional<std::string> arg_path(argparser, "image", "Path to image file");
  args::ValueFlag<int> arg_rows(argparser, "row size", "The size of one row (=5)", {'r', "row"});
  args::ValueFlag<int> arg_cols(argparser, "col size", "The size of one col (=5)", {'c', "col"});
  args::ValueFlag<double> arg_threshold(argparser, "threshold", "threshold (=150)", {'t', "threshold"});
  args::Flag arg_define_shorten(argparser, "Shorten using #define", "Use #define to shorten tokens", {'d', "emit-define"});
  args::ValueFlag<int> arg_desth(argparser, "define threshold", "Minimum length of token (=5)", {"emit-define-threshold"});
  args::Flag arg_verbose(argparser, "verbose", "Print verbose output and show images in process", {'v', "verbose"});
  args::Flag arg_invert(argparser, "invert", "invert image", {'i', "invert"});
  try{
      argparser.ParseCLI(argc, argv);
  } catch (args::Help){
      std::cout << argparser;
      return 0;
  } catch (args::ParseError e){
      std::cerr << e.what() << std::endl;
      std::cerr << argparser;
      return -1;
  }
  if(!arg_path){
    std::cerr << "Specify a path to image file." << std::endl;
    std::cerr << argparser;
    return -1;
  }
  if(!arg_source){
    std::cerr << "Specify a path to image file." << std::endl;
    std::cerr << argparser;
    return -1;
  }

  // rows : cols == canny_image.rows : canny_image.cols
  auto const rows = arg_rows ? args::get(arg_rows) : 5;
  auto const cols = arg_cols ? args::get(arg_cols) : 5;
  auto const path = args::get(arg_path);
  auto const th = arg_threshold ? args::get(arg_threshold) : 150;
  auto const desth = arg_desth ? args::get(arg_desth) : 5;
  bool const define_shorten = arg_define_shorten;
  bool const verbose = arg_verbose;
  bool const invert = arg_invert;

  auto raw_image = cv::imread(path, 0);
  if(!raw_image.data){
    std::cerr << "Failed to load supplied image: " << path << std::endl;
    return -1;
  }

  cv::Mat th_image;
  try{
	   cv::threshold(raw_image, th_image, th, 255, cv::THRESH_BINARY);
  }catch(cv::Exception& ex){
    std::cerr << "Failed to apply threshold." << std::endl;
    std::cerr << ex.what() << std::endl;
    return -1;
  }

  if(invert){
      cv::Mat iv_image;
      try{
          cv::bitwise_not(th_image, iv_image);
      }catch(cv::Exception& ex){
          std::cerr << "Failed to invert image" << std::endl;
          std::cerr << ex.what() << std::endl;
          return -1;
      }
      th_image = iv_image;
  }

  int cn = std::floor(th_image.cols / cols) * cols;
  int rn = std::floor(th_image.rows / rows) * rows;
  assert(!(rn % rows || cn % cols));

  if(verbose){
    std::cerr << "Resized image: " << rn << 'x' << cn << std::endl;
    cv::imshow("threshold", th_image);
    cv::waitKey(0);
  }

  cv::Mat src_image;
  try{
    cv::resize(th_image, src_image, cv::Size(cn, rn));
  }catch(cv::Exception& ex){
   std::cerr << "Failed to resize image." << std::endl;
   std::cerr << ex.what() << std::endl;
   return -1;
  }

  std::vector<std::string> tokens{};
  tokenize(std::back_inserter(tokens), args::get(arg_source));

  if(tokens.empty()){
    std::cerr << "Failed to tokenize." << std::endl;
    return -1;
  }

  std::vector<std::string> prep{};
  std::map<std::string, std::string> defmap{};
  auto it = std::begin(tokens);
  auto last = std::remove_if(tokens.begin(), tokens.end(),
    [&prep](auto&& x) { bool c = *std::cbegin(x) == '#'; if(c) prep.push_back(x); return c; });
  int itr = 0;
  std::stringstream result;
  for(int y = 0; y < src_image.rows; y+=rows){
    for(int x = 0; x < src_image.cols; x+=cols){
      auto part_image = src_image(cv::Rect(x, y, cols, rows));

      int b = std::count_if(part_image.begin<unsigned char>(), part_image.end<unsigned char>(), [](auto x) -> bool { return x; });
      if(b > part_image.total() / 2){
        if(it == last){
          result << "//";
          x += cols * 2;
        }else{
          if(*std::cbegin(*it) == '#'){
            result << '\n' << *it << '\n';
          }else if(it->length() > desth && *it->begin() != '"' && *it->end() != '"' && define_shorten){
            if(!defmap.count(*it)){
              std::string key;
              do{
                int p = itr++;
                if(!p)
                  key = "a";
                else{
                  int i;
                  char t[64] = { };
                  for(i = 62; p; --i) {
                    t[i] = p % 26 + 'a';
                    p /= 26;
                  }
                  key = std::string(t + i + 1);
                }
                if(*(std::cend(*it) - 1) == ' ')
                  key += " ";
              }while(std::find(tokens.begin(), tokens.end(), key) != tokens.end());
              result << key;
              defmap.insert({*it, key});
            }else{
              result << defmap[*it];
            }
          }else{
            result << *it;
            x += it->length() * cols;
          }
          it++;
        }
      }else
        result << " ";
    }
    result << std::endl;
  }

  if(it != last){
    std::copy(it,
               last,
               std::ostream_iterator<std::string>(result));
  }

  std::copy(prep.begin(),
             prep.end(),
             std::ostream_iterator<std::string>(std::cout));

  std::cout << std::endl;

  for(auto const& p : defmap){
    std::cout << "#define " << p.second << " " << p.first << std::endl;
  }

  std::copy(std::istreambuf_iterator<char>(result),
             std::istreambuf_iterator<char>(),
             std::ostreambuf_iterator<char>(std::cout));

  return 0;
}
