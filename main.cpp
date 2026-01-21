#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdint>
#include <limits>
#include <algorithm>
#include <random>
#include <numeric>
#include <cmath>

using namespace std;

#pragma pack(push, 1)
struct BmpBaslik {
    uint16_t file_type; uint32_t file_size; uint16_t reserved1; uint16_t reserved2;
    uint32_t offset_data; uint32_t size; int32_t  width; int32_t  height;
    uint16_t planes; uint16_t bit_count; uint32_t compression; uint32_t size_image;
    int32_t  x_pixels_per_meter; int32_t  y_pixels_per_meter; uint32_t colors_used; uint32_t colors_important;
};
#pragma pack(pop)

class BmpIsleyici {
private:
    BmpBaslik baslik;
    vector<uint8_t> pikselVerisi;
    vector<uint8_t> ekVeri;
    string aktifDosyaIsmi;
    const string IMZA = "#GIZLI#"; 
    const int HEADER_BOYUTU = 12; 

    vector<uint8_t> dosyayiBinaryOku(const string& yol) {
        ifstream dosya(yol, ios::binary | ios::ate);
        if (!dosya) return {};
        streamsize boyut = dosya.tellg();
        dosya.seekg(0, ios::beg);
        vector<uint8_t> tampon(boyut);
        if (dosya.read((char*)tampon.data(), boyut)) return tampon;
        return {};
    }

    bool veriyiDosyayaYaz(const string& ad, const vector<uint8_t>& veri) {
        ofstream dosya(ad, ios::binary);
        if (!dosya) return false;
        dosya.write((char*)veri.data(), veri.size());
        return true;
    }

    void xorIslemi(vector<uint8_t>& veri, const string& parola) {
        if (parola.empty()) return;
        for (size_t i = 0; i < veri.size(); i++) {
            veri[i] ^= (uint8_t)parola[i % parola.length()];
        }
    }

    unsigned int tohumUret(const string& parola) {
        unsigned int seed = 0;
        for (char c : parola) seed = seed * 31 + c;
        return seed;
    }

    vector<size_t> akilliIndeksleriGetir(const string& parola, int bitDerinligi, size_t gerekenPikselSayisi) {
        bool adaptiveModAktif = (bitDerinligi <= 5);
        if (!adaptiveModAktif) {
             vector<size_t> indeksler(pikselVerisi.size());
             iota(indeksler.begin(), indeksler.end(), 0);
             mt19937 g(tohumUret(parola));
             shuffle(indeksler.begin(), indeksler.end(), g);
             return indeksler;
        }

        int bytesPerPixel = baslik.bit_count / 8;
        if (bytesPerPixel < 1) bytesPerPixel = 1;
        uint8_t msbMaskesi = ~((1 << bitDerinligi) - 1); 

        vector<pair<int, size_t>> pikselSkorlari;
        pikselSkorlari.reserve(pikselVerisi.size());

        for (size_t i = 0; i < pikselVerisi.size(); ++i) {
            int fark = 0;
            if (i >= bytesPerPixel) {
                uint8_t val1 = pikselVerisi[i] & msbMaskesi;
                uint8_t val2 = pikselVerisi[i - bytesPerPixel] & msbMaskesi;
                fark = abs(val1 - val2);
            }
            pikselSkorlari.push_back({fark, i});
        }

        stable_sort(pikselSkorlari.begin(), pikselSkorlari.end(), 
            [](const pair<int, size_t>& a, const pair<int, size_t>& b) {
                return a.first > b.first; 
            });

        size_t havuzBoyutu = gerekenPikselSayisi * 2; // Çakışma ihtimaline karşı tedbir alındı.
        if (havuzBoyutu > pikselVerisi.size()) havuzBoyutu = pikselVerisi.size();

        vector<size_t> secilenIndeksler;
        secilenIndeksler.reserve(havuzBoyutu);
        for(size_t i=0; i<havuzBoyutu; i++) secilenIndeksler.push_back(pikselSkorlari[i].second);

        mt19937 g(tohumUret(parola));
        shuffle(secilenIndeksler.begin(), secilenIndeksler.end(), g);
        return secilenIndeksler;
    }
    
    vector<size_t> globalShuffleIndeksleri(const string& parola) {
        vector<size_t> indeksler(pikselVerisi.size());
        iota(indeksler.begin(), indeksler.end(), 0);
        mt19937 g(tohumUret(parola));
        shuffle(indeksler.begin(), indeksler.end(), g);
        return indeksler;
    }

    void intToBytes(uint32_t n, vector<uint8_t>& dest) {
        dest.push_back(n & 0xFF); dest.push_back((n >> 8) & 0xFF);
        dest.push_back((n >> 16) & 0xFF); dest.push_back((n >> 24) & 0xFF);
    }

    uint32_t bytesToInt(const vector<uint8_t>& src, int offset) {
        uint32_t n = 0;
        n |= src[offset]; n |= (src[offset + 1] << 8);
        n |= (src[offset + 2] << 16); n |= (src[offset + 3] << 24);
        return n;
    }

public:
    size_t getPikselSayisi() const { return pikselVerisi.size(); }

    bool dosyaAc(const string& dosyaAdi) {
        aktifDosyaIsmi = dosyaAdi;
        ifstream dosya(dosyaAdi, ios::binary);
        if (!dosya) return false;
        dosya.read(reinterpret_cast<char*>(&baslik), sizeof(BmpBaslik));
        if (baslik.file_type != 0x4D42) return false; 
        long boslukBoyutu = baslik.offset_data - sizeof(BmpBaslik);
        if (boslukBoyutu > 0) {
            ekVeri.resize(boslukBoyutu);
            dosya.read(reinterpret_cast<char*>(ekVeri.data()), boslukBoyutu);
        } else ekVeri.clear();
        uint32_t veriBoyutu = baslik.size_image;
        if (veriBoyutu == 0) veriBoyutu = baslik.file_size - baslik.offset_data;
        dosya.seekg(baslik.offset_data, ios::beg); 
        pikselVerisi.resize(veriBoyutu);
        dosya.read(reinterpret_cast<char*>(pikselVerisi.data()), veriBoyutu);
        dosya.close();
        return true;
    }

    void dosyayiKaydet() {
        ofstream dosya(aktifDosyaIsmi, ios::binary | ios::out | ios::trunc);
        if (dosya) {
            dosya.write(reinterpret_cast<char*>(&baslik), sizeof(BmpBaslik));
            if (!ekVeri.empty()) dosya.write(reinterpret_cast<char*>(ekVeri.data()), ekVeri.size());
            dosya.write(reinterpret_cast<char*>(pikselVerisi.data()), pikselVerisi.size());
            dosya.close();
            cout << "Resim guncellendi: " << aktifDosyaIsmi << endl;
        } else cout << "Hata: Dosyaya yazilamadi!" << endl;
    }

    bool gizleDosya(string gizlenecekDosyaYolu, int veriBitDerinligi, const string& parola) {
        vector<uint8_t> dosyaIcerigi = dosyayiBinaryOku(gizlenecekDosyaYolu);
        if (dosyaIcerigi.empty()) return false;

        vector<uint8_t> hamPaket;
        for(char c : gizlenecekDosyaYolu) hamPaket.push_back(c);
        hamPaket.push_back(0); 
        hamPaket.insert(hamPaket.end(), dosyaIcerigi.begin(), dosyaIcerigi.end());
        xorIslemi(hamPaket, parola);

        vector<uint8_t> headerBytes;
        for(char c : IMZA) headerBytes.push_back(c);
        headerBytes.push_back((uint8_t)veriBitDerinligi);
        uint32_t paketBoyutu = hamPaket.size();
        intToBytes(paketBoyutu, headerBytes);

        // --- ÇAKIŞMA KONTROLÜ ---
        // Hangi piksellerin dolu olduğu takip edilir.
        vector<bool> pikselDoluMu(pikselVerisi.size(), false);

        // 1. HEADER YAZMA
        vector<size_t> globalIndeksler = globalShuffleIndeksleri(parola);
        int pikselSayaci = 0;
        
        for (uint8_t byte : headerBytes) {
            for (int b = 0; b < 8; ++b) {
                size_t idx = globalIndeksler[pikselSayaci++];
                
                // Piksel işaretleme
                pikselDoluMu[idx] = true;

                pikselVerisi[idx] &= 0xFE; 
                pikselVerisi[idx] |= ((byte >> b) & 1);
            }
        }

        // 2. BODY YAZMA
        size_t gerekenPiksel = (hamPaket.size() * 8 + veriBitDerinligi - 1) / veriBitDerinligi;
        vector<size_t> adaptiveIndeksler = akilliIndeksleriGetir(parola, veriBitDerinligi, gerekenPiksel);
        
        int paketIndex = 0, bitIndex = 0, adaptiveSayac = 0;
        while (paketIndex < hamPaket.size()) {
            if (adaptiveSayac >= adaptiveIndeksler.size()) break;
            
            size_t idx = adaptiveIndeksler[adaptiveSayac++];
            
            // --- Eğer bu piksel Header tarafından kullanıldıysa ATLA ---
            if (pikselDoluMu[idx]) continue;

            uint8_t maske = ~((1 << veriBitDerinligi) - 1);
            pikselVerisi[idx] &= maske;
            uint8_t veriParcasi = 0;
            for (int b = 0; b < veriBitDerinligi; ++b) {
                if (paketIndex >= hamPaket.size()) break;
                uint8_t byte = hamPaket[paketIndex];
                int bit = (byte >> bitIndex) & 1;
                veriParcasi |= (bit << b);
                bitIndex++;
                if (bitIndex == 8) { bitIndex = 0; paketIndex++; }
            }
            pikselVerisi[idx] |= veriParcasi;
        }
        return true;
    }

    bool cozVeDosyaCikar(const string& parola) {
        // Hangi piksellerin dolu olduğunu takip et (Okurken de atlamak için)
        vector<bool> pikselDoluMu(pikselVerisi.size(), false);

        // 1. Header Oku
        vector<size_t> globalIndeksler = globalShuffleIndeksleri(parola);
        vector<uint8_t> okunanHeader(HEADER_BOYUTU);
        int pikselSayaci = 0;
        vector<size_t> temizlenecekPikseller;

        for (int i = 0; i < HEADER_BOYUTU; ++i) {
            uint8_t byte = 0;
            for (int b = 0; b < 8; ++b) {
                size_t idx = globalIndeksler[pikselSayaci++];
                
                // Okuduğumuz header pikselini işaretle
                pikselDoluMu[idx] = true;
                temizlenecekPikseller.push_back(idx);

                byte |= ((pikselVerisi[idx] & 1) << b);
            }
            okunanHeader[i] = byte;
        }

        string okunanImza = "";
        for(int i=0; i<IMZA.length(); i++) okunanImza += (char)okunanHeader[i];
        if (okunanImza != IMZA) return false;

        int bit = okunanHeader[IMZA.length()];
        uint32_t boyut = bytesToInt(okunanHeader, IMZA.length() + 1);
        if (bit < 1 || bit > 8) return false;

        // 2. Body Oku (Atlama Mantığı ile)
        size_t gerekenBodyBit = (size_t)boyut * 8;
        size_t gerekenPiksel = (gerekenBodyBit + bit - 1) / bit;
        
        vector<size_t> adaptiveIndeksler = akilliIndeksleriGetir(parola, bit, gerekenPiksel);
        vector<uint8_t> sifreliPaket(boyut);
        int yazilanByte = 0, bitSayac = 0, adaptiveSayac = 0;
        uint8_t anlikByte = 0;

        while (yazilanByte < boyut) {
            if (adaptiveSayac >= adaptiveIndeksler.size()) break;
            size_t idx = adaptiveIndeksler[adaptiveSayac++];

            // --- Çakışmayı önlemek için Header piksellerini burada da ATLA ---
            if (pikselDoluMu[idx]) continue;

            temizlenecekPikseller.push_back(idx); 
            uint8_t val = pikselVerisi[idx] & ((1 << bit) - 1);
            for (int b = 0; b < bit; ++b) {
                int veriBit = (val >> b) & 1;
                anlikByte |= (veriBit << bitSayac);
                bitSayac++;
                if (bitSayac == 8) {
                    sifreliPaket[yazilanByte++] = anlikByte;
                    anlikByte = 0; bitSayac = 0;
                    if (yazilanByte >= boyut) break;
                }
            }
        }

        for (size_t idx : temizlenecekPikseller) pikselVerisi[idx] = 255; 
        xorIslemi(sifreliPaket, parola);

        string orijinalIsim = "";
        size_t dataBaslangic = 0;
        for (size_t i = 0; i < sifreliPaket.size(); ++i) {
            if (sifreliPaket[i] == 0) { dataBaslangic = i + 1; break; }
            orijinalIsim += (char)sifreliPaket[i];
        }

        if (dataBaslangic == 0 || dataBaslangic >= sifreliPaket.size()) return false;
        
        vector<uint8_t> dosyaData;
        dosyaData.insert(dosyaData.end(), sifreliPaket.begin() + dataBaslangic, sifreliPaket.end());
        
        string cikisIsmi = "GIZLI_" + orijinalIsim;
        if (veriyiDosyayaYaz(cikisIsmi, dosyaData)) {
            cout << "\n[BASARILI] Dosya cikarildi: " << cikisIsmi << endl;
            return true;
        }
        return false;
    }
};

int main() {
    BmpIsleyici arac;
    int secim;
    string resimYolu, hedefDosya, parola;

    cout << "=== STEALTH STEGANOGRAPHY ===" << endl;
    cout << "1. Dosya Gizle" << endl;
    cout << "2. Dosya Cikar" << endl;
    cout << "Secim: ";
    cin >> secim;

    cout << "Tasiyici Resim: ";
    cin >> resimYolu;
    if (!arac.dosyaAc(resimYolu)) { cout << "Dosya hatasi!" << endl; return 1; }

    if (secim == 1) {
        cout << "Gizlenecek Dosya: "; 
        cin.ignore(numeric_limits<streamsize>::max(), '\n'); 
        getline(cin, hedefDosya);

        int bit; cout << "Kalite (1-5 bit onerilir, Max 8): "; cin >> bit; 
        if (bit<1) bit=1; if(bit>8) bit=8;
        
        cout << "Parola: "; cin >> parola;
        cout << "Analiz ve Gizleme basliyor..." << endl;
        
        if (arac.gizleDosya(hedefDosya, bit, parola)) {
            arac.dosyayiKaydet();
            cout << "ISLEM TAMAM." << endl;
        }
    } else if (secim == 2) {
        cout << "Parola: "; cin >> parola;
        if (arac.cozVeDosyaCikar(parola)) arac.dosyayiKaydet();
        else cout << "Hata: Dosya/Parola gecersiz." << endl;
    }
    return 0;
}
