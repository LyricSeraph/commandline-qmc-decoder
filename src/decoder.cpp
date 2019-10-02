#include "seed.hpp"
#include <cassert>
#include <boost/filesystem.hpp>
#include <fstream>
#include <iostream>
#include <mutex>
#include <regex>
#include <thread>
#include <vector>

using std::fstream;
using std::ios;
using std::move;
using std::regex;
using std::regex_replace;
using std::string;
using std::thread;
using std::vector;
using std::cout;
using std::mutex;
using std::lock_guard;
using std::mutex;

namespace fs = boost::filesystem;

mutex mtx;
bool force_override = false, is_recursive = false, decode_flac = false, decode_mp3 = false;
const regex reg_mp3{"^.+\\.(qmc3|qmc0)$"};
const regex reg_flac{"^.+\\.qmcflac$"};

inline void safe_out(const string &data)
{
    lock_guard<std::mutex> lock(mtx);
    cout << data << std::endl;
}
inline void safe_out(const char *data)
{
    lock_guard<std::mutex> lock(mtx);
    cout << data << std::endl;
}
bool match_format(string file_path, bool match_mp3, bool match_flac) {
    if (match_mp3 && regex_match(file_path, reg_mp3)) return true;
    if (match_flac && regex_match(file_path, reg_flac)) return true;
    return false;
}

void process(const string &file_path)
{
    safe_out("[INFO] decode file " + file_path);
    fstream infile(file_path, ios::in | ios::binary);
    if (!infile.is_open())
    {
        safe_out("[ERROR] qmc file read error");
        return;
    }

    string outloc(file_path);
    const regex mp3_regex{"\\.(qmc3|qmc0)$"}, flac_regex{"\\.qmcflac$"};
    auto mp3_outloc = regex_replace(outloc, mp3_regex, ".mp3");
    auto flac_outloc = regex_replace(outloc, flac_regex, ".flac");

    assert(mp3_outloc != flac_outloc);
    outloc = (outloc != mp3_outloc ? mp3_outloc : flac_outloc);
    if (!force_override && fs::exists(fs::path(outloc))) {
        safe_out("[WARN] output file exist: " + outloc);
        return ;
    }

    auto len = infile.seekg(0, ios::end).tellg();
    infile.seekg(0, ios::beg);
    char *buffer = new char[len];

    infile.read(buffer, len);
    infile.close();

    qmc_decoder::seed seed_;
    for (int i = 0; i < len; ++i)
    {
        buffer[i] = seed_.NextMask() ^ buffer[i];
    }

    fstream outfile(outloc.c_str(), ios::out | ios::binary);

    if (outfile.is_open())
    {
        outfile.write(buffer, len);
        outfile.close();
    }
    else
    {
        safe_out("[ERROR] open dump file error: " + outloc);
    }
    delete[] buffer;
}

void scan_directory(string dir_path, vector<string> & qmc_paths)
{
    fs::path dir_file(dir_path);
    if (is_recursive) {
        for (auto iter : fs::recursive_directory_iterator(dir_file))
        {
            auto file_path = iter.path().string();
            if (fs::is_regular_file(iter) && match_format(file_path, decode_mp3, decode_flac))
            {
                qmc_paths.emplace_back(file_path);
            }
        };
    } else {
        for (auto iter : fs::directory_iterator(dir_file))
        {
            auto file_path = iter.path().string();
            if (fs::is_regular_file(iter) && match_format(file_path, decode_mp3, decode_flac))
            {
                qmc_paths.emplace_back(file_path);
            }
        };

    }

}

void process_files(vector<string> & qmc_paths) {
    const auto n_thread = thread::hardware_concurrency();
    vector<thread> td_group;

    for (size_t i = 0; i < n_thread - 1; ++i)
    {
        td_group.emplace_back([&qmc_paths, &n_thread](int index) {
            for (size_t j = index; j < qmc_paths.size(); j += n_thread)
            {
                process(qmc_paths[j]);
            }
        }, i);
    }
    for (size_t j = n_thread - 1; j < qmc_paths.size(); j += n_thread)
    {
        process(qmc_paths[j]);
    }

    for (auto &&td : td_group)
    {
        td.join();
    }
}

int main(int argc, char ** argv) {
    vector<string> files;
    int ch;
    while((ch = getopt(argc, argv, "orMFf:")) != -1)
    {
        switch(ch)
        {
            case 'o':
                force_override = true;
                break;
            case 'r':
                is_recursive = true;
                break;
            case 'M':
                decode_mp3 = true;
                break;
            case 'F':
                decode_flac = true;
                break;
            case 'f':
                files.push_back(string(optarg));
                while (optind < argc) {
                    files.push_back(string(argv[optind++]));
                }
                break;
        }
    }
    if (files.size() == 0) {
        safe_out("Usage: " + string(argv[0]) + " [-o] [-r] (-M|-F) -f [<file> ...]");
        safe_out("    -o: override exist output files.");
        safe_out("    -r: recursively scan directories for matching files.");
        safe_out("    -M: matching QMC3/QMC0 files");
        safe_out("    -F: matching QMCFLAC files");
        safe_out("    -f: directories/files to decode");
        return 0;
    }
    if (!decode_mp3 && !decode_flac) {
        safe_out("[ERROR] No decode format is assigned, please use at least one of -M for decoding qmc3/0 to MP3, or -F for decoding qmcflac to flac.");
        return 0;
    }
    vector<string> files_to_process;
    for (auto file_path : files) {
        fs::path file(file_path);
        if (fs::is_regular_file(file) && match_format(file_path, decode_mp3, decode_flac)) {
            files_to_process.emplace_back(file_path);
        } else if (fs::is_directory(file)) {
            scan_directory(file_path, files_to_process);
        }
    }
    process_files(files_to_process);
    return 0;
}
