#include <iostream>
#include <wx/string.h>
#include <wx/filefn.h>
#include <wx/filename.h>
#include "Tasks/ReadIndexTask.h"
#include "DatIndex.h"
#include "DatFile.h"
#include "Exporter.h"
#include "Readers/ImageReader.h"
#include "Tasks/ScanDatTask.h"
#include "Readers/asndMP3Reader.h"
#include "Readers/PackedSoundReader.h"
#include <thread>
#include <chrono>
#include <mutex>
#include <future>

using namespace std::chrono_literals;
using namespace gw2b;

void appendPaths(wxFileName &p_path, const DatIndexCategory &p_category) {
    auto parent = p_category.parent();
    if (parent) {
        appendPaths(p_path, *parent);
    }
    p_path.AppendDir(p_category.name());
}

auto extension(ANetFileType type) {
    switch (type) {
        case ANFT_ATEX:
        case ANFT_ATTX:
        case ANFT_ATEC:
        case ANFT_ATEP:
        case ANFT_ATEU:
        case ANFT_ATET:
        case ANFT_DDS:
        case ANFT_JPEG:
        case ANFT_WEBP:
        case ANFT_PNG:
        case ANFT_BitmapFontFile:
            return wxT("png");
            break;
        case ANFT_Model:
            return wxT("obj");
            break;
        case ANFT_StringFile:
            return wxT("csv");
            break;
        case ANFT_GameContent:
            return wxT("xml");
            break;
        case ANFT_PackedOgg:
        case ANFT_Ogg:
            return wxT("ogg");
            break;
        case ANFT_PackedMP3:
        case ANFT_asndMP3:
        case ANFT_MP3:
        case ANFT_Bank:
            return wxT("mp3");
            break;
        case ANFT_EULA:
        case ANFT_TEXT:
        case ANFT_UTF8:
            return wxT("txt");
            break;
        case ANFT_Bink2Video:
            return wxT("bk2");
            break;
        case ANFT_DLL:
            return wxT("dll");
            break;
        case ANFT_EXE:
            return wxT("exe");
            break;
        default:
            return wxT("raw"); // todo
            break;
    }
}

bool writeFile(const Array<byte> &p_data, wxFileName &m_filename) {
    // Open file for writing
    wxFile file(m_filename.GetFullPath(), wxFile::write);
    if (file.IsOpened()) {
        file.Write(p_data.GetPointer(), p_data.GetSize());
    } else {
        std::cerr << wxString::Format(wxT("Failed to open the file %s for writing."), m_filename.GetFullPath())
                  << std::endl;
        return false;
    }
    file.Close();
    return true;
}

void writeImage(wxImage p_image, wxFileName &m_filename) {
    // Write the png to memory
    wxMemoryOutputStream stream;
    if (!p_image.SaveFile(stream, wxBITMAP_TYPE_PNG)) {
        std::cerr << "Failed to write png to memory." << std::endl;
        return;
    }

    // Reset the position of the stream
    auto buffer = stream.GetOutputStreamBuffer();
    buffer->Seek(0, wxSeekMode::wxFromStart);

    // Read the data from the stream and into a buffer
    Array<byte> data(buffer->GetBytesLeft());
    buffer->Read(data.GetPointer(), data.GetSize());

    // Write to file
    writeFile(data, m_filename);
}

void exportSound(FileReader *p_reader, const wxString &p_entryname, ANetFileType file_type, wxFileName &m_filename) {
    Array<byte> data;
    if (file_type == ANFT_asndMP3) {
        auto asndMP3 = dynamic_cast<asndMP3Reader *>( p_reader );
        if (!asndMP3) {
            std::printf("Entry %s is not a sound file.\n", p_entryname.c_str().AsChar());
            return;
        }
        // Get sound data
        data = asndMP3->getMP3Data();

    } else if (
            (file_type == ANFT_PackedMP3) ||
            (file_type == ANFT_PackedOgg)
            ) {
        auto packedSound = dynamic_cast<PackedSoundReader *>( p_reader );

        if (!packedSound) {
            std::printf("Entry %s is not a sound file.\n", p_entryname.c_str().AsChar());
            return;
        }
        // Get sound data
        data = packedSound->getSoundData();
    }
    // Write to file
    writeFile(data, m_filename);
}

void exportImage(FileReader *p_reader, const wxString &p_entryname, wxFileName &m_filename) {
    // Bail if not an image
    auto imgReader = dynamic_cast<ImageReader *>( p_reader );
    if (!imgReader) {
        std::cerr << (wxString::Format(wxT("Entry %s is not an image."), p_entryname)) << std::endl;
        return;
    }

    // Get image in wxImage
    auto imageData = imgReader->getImage();

    if (!imageData.IsOk()) {
        std::cerr << (wxString::Format(wxT("imgReader->getImage( ) error in entry %s."), p_entryname)) << std::endl;
        return;
    }

    writeImage(imageData, m_filename);
}

void addCategoryEntriesToArray(Array<const DatIndexEntry *> &p_array, uint &p_index,
                               const DatIndexCategory &p_category) {
    // Loop through subcategories
    for (uint i = 0; i < p_category.numSubCategories(); i++) {
        addCategoryEntriesToArray(p_array, p_index, *p_category.subCategory(i));
    }

    // Loop through entries
    for (uint i = 0; i < p_category.numEntries(); i++) {
        p_array[p_index++] = p_category.entry(i);
    }
}

int main(int argc, char **argv) {
    if (argc < 3) {
        std::cerr << "2 arguments are expected: dat file path followed by output directory" << std::endl;
        return 1;
    }

    auto dat_path = wxString::FromUTF8Unchecked(argv[1]);
    auto out_dir = wxString::FromUTF8Unchecked(argv[2]);

    auto dat_file = DatFile();
    if (!dat_file.open(dat_path)) {
        std::fprintf(stderr, "Failed to open file: %s\n", dat_path.c_str().AsChar());
        return 1;
    }

    auto index = std::make_shared<DatIndex>();
    auto dat_ts = wxFileModificationTime(dat_path);

    DatIndexReader indexReader(*index);
    if (!indexReader.open(wxString("gw2dat.idx")) || index->datTimestamp() != dat_ts) {
        auto scan_dat = ScanDatTask(index, dat_file);

        if (!scan_dat.init()) {
            std::cerr << "failed to initialize read dat task" << std::endl;
            return 1;
        }
        index->setDatTimestamp(dat_ts);

        auto thr = std::thread([&] {
            while (!scan_dat.isDone()) {
                scan_dat.perform();
            }
        });


        std::future<void> future = std::async(std::launch::async, [&]() {
            if (thr.joinable()) thr.join();
        });
        for (;;) {
            if (future.wait_for(5s) == std::future_status::ready) {
                break;
            }

            std::printf("Scan Dat %7u / %7u\n", scan_dat.currentProgress(), scan_dat.maxProgress());
        }
        std::cout << "Scan Dat    Done" << std::endl;

        DatIndexWriter indexWriter(*index);
        indexWriter.open(wxString("gw2dat.idx"));
        indexWriter.write(100000000);
    }
    indexReader.read(100000000);

    auto textures = index->findCategory(wxString("Textures"));
    auto ui = textures->findSubCategory(wxString("UI Textures"));

    auto entries = Array<const DatIndexEntry *>(ui->numEntries(true));
    uint max = 0;
    addCategoryEntriesToArray(entries, max, *ui);
    wxInitAllImageHandlers();

    std::mutex mutex_index;
    std::mutex mutex_dir;
    uint i = 0;

    auto num_threads = std::thread::hardware_concurrency();
    std::vector<std::thread> threads;
    for (auto t = 0; t < num_threads; t++) {
        threads.emplace_back([&] {
            auto dat_file = DatFile();
            if (!dat_file.open(dat_path)) {
                std::fprintf(stderr, "Failed to open file: %s\n", dat_path.c_str().AsChar());
                std::exit(1);
            }

            for (i = 0; i < max; i++) {
                uint idx;
                {
                    std::lock_guard<std::mutex> lock(mutex_index);
                    if (i >= max) {
                        break;
                    }
                    idx = i++;
                }

                auto entry = entries[idx];

                auto entry_file_name = wxFileName();
                auto file_type = entry->fileType();
                auto ext = extension(file_type);

                // Set file path
                entry_file_name.SetPath(out_dir);
                // Set file name
                entry_file_name.SetName(entry->name());
                // Set file extension
                entry_file_name.SetExt(wxString(ext));



                // Appen category name as path
                appendPaths(entry_file_name, *entry->category());

                // Create directory if not exist
                {
                    std::lock_guard<std::mutex> lock(mutex_dir);
                    if (!entry_file_name.DirExists()) {
                        entry_file_name.Mkdir(511, wxPATH_MKDIR_FULL);
                    }
                }

                auto entryData = dat_file.readFile(entry->mftEntry());
                // Valid data?
                if (!entryData.GetSize()) {
                    std::fprintf(stderr, "Failed to read file: %s\n", entry_file_name.GetFullName().c_str().AsChar());
                    std::exit(1);
                }

                entry_file_name.SetExt(wxString(ext));

                // Identify file type
                dat_file.identifyFileType(entryData.GetPointer(), entryData.GetSize(), file_type);

                auto reader = FileReader::readerForData(entryData, dat_file, file_type);

                if (reader) {

                    switch (file_type) {
                        case ANFT_ATEX:
                        case ANFT_ATTX:
                        case ANFT_ATEC:
                        case ANFT_ATEP:
                        case ANFT_ATEU:
                        case ANFT_ATET:
                        case ANFT_DDS:
                        case ANFT_JPEG:
                        case ANFT_WEBP:
                            exportImage(reader, entry->name(), entry_file_name);
                            break;
                        case ANFT_StringFile:
                            std::cerr << "string" << std::endl;
                            //exportString( reader, entry->name( ), entry_file_name );
                            std::exit(1);
                            break;
                        case ANFT_EULA:
                            std::cerr << "eula" << std::endl;
                            //exportEula( reader, entry->name( ), entry_file_name );
                            std::exit(1);
                            break;
                        case ANFT_PackedMP3:
                        case ANFT_PackedOgg:
                        case ANFT_asndMP3:
                            exportSound(reader, entry->name(), file_type, entry_file_name);
                            break;
                        case ANFT_Bank:
                            std::cerr << "bank" << std::endl;
                            //exportSoundBank( reader, entry->name( ), entry_file_name );
                            std::exit(1);
                            break;
                        case ANFT_Model:
                            std::cerr << "model" << std::endl;
                            //exportModel( reader, entry->name( ), entry_file_name );
                            std::exit(1);
                            break;
                        case ANFT_GameContent:
                            std::cerr << "content" << std::endl;
                            //exportGameContent( reader, entry->name( ), entry_file_name );
                            std::exit(1);
                            break;
                        case ANFT_BitmapFontFile:
                            std::cerr << "font" << std::endl;
                            //exportBitmapFont( reader, entry->name( ), entry_file_name );
                            std::exit(1);
                            break;
                        default:
                            //entryData = reader->rawData( );
                            writeFile(entryData, entry_file_name);
                            break;
                    }

                    deletePointer(reader);
                } else {
                    writeFile(entryData, entry_file_name);
                }
            }
        });
    }

    for (auto &thr: threads) {
        std::future<void> future = std::async(std::launch::async, [&]() {
            if (thr.joinable()) thr.join();
        });
        for (;;) {
            if (future.wait_for(1s) == std::future_status::ready) {
                break;
            }

            std::printf("Export   %7u / %7u\n", i, max);
        }
    }
    std::cout << "Export      Done" << std::endl;

    return 0;
}
