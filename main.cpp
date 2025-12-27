#include <iostream>
#include <fstream>  // Binary modda dosya okuma/yazma işlemleri için gerekli
#include <vector>   // Resim piksellerini dinamik bir dizide tutmak için gerekli
#include <string>
#include <cstdint>  // uint8_t, uint32_t gibi sabit 
#include <limits>   // cin.ignore kullanarak getline fonksiyonunda buffer temizliği yapmak için.

using namespace std;

// BMP Baslik Yapisi (Standart)
#pragma pack(push, 1)
/*
    C++ derleyicileri yapıları bellekte hızlı erişim için 4 veya 8 byte katlarına
    tamamlar. Ancak BMP dosyasının başlığı diskte ardışık baytlar halinde. Eğer dolgu
    (padding) yapılırsa dosya başlığı okumada problem yaşarım. pack(1) diyerek araya
    hiç boşluk koyma dedim.
*/
// BMP başlık struct ı  BMP dosyasının ilk 54 baytını temsil ediyor.
// Dosya tipi (BM imzası) genişlik yükseklik dosya boyutu gibi özel verileri tutuyor bu kısım.
struct BmpBaslik {
    uint16_t file_type;      
    uint32_t file_size;      
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t offset_data;    
    uint32_t size;           
    int32_t  width;
    int32_t  height;
    uint16_t planes;
    uint16_t bit_count;      
    uint32_t compression;
    uint32_t size_image;     
    int32_t  x_pixels_per_meter;
    int32_t  y_pixels_per_meter;
    uint32_t colors_used;
    uint32_t colors_important;
};
#pragma pack(pop)

class BmpIsleyici {
private:
    BmpBaslik baslik;
    vector<uint8_t> pikselVerisi;
    vector<uint8_t> ekVeri;
    string aktifDosyaIsmi;
    
    /*
        Buradaki imza yaklaşımı ile üzerine gizli mesaj yazılmamış ham
        dosyaları ayırt ediyorum.
        Yazdığım her gizli mesajın başına #GIZLI# başlığını da ekliyorum
        böylece kodum bu mesajı görürse deşifre işleminde dosyayı dikkate
        alıyor ama eğer göremezse hiç okumuyor direkt bu bmp dosyasında mesaj
        yok diyor.
    */
    const string IMZA = "#GIZLI#"; 

public:
    bool dosyaAc(const string& dosyaAdi) {
        aktifDosyaIsmi = dosyaAdi;
        ifstream dosya(dosyaAdi, ios::binary);
        if (!dosya) return false;

        /*
            reinterpret_cast yapısı ile başlığın adresini char olarak okumaya zorladım
            çünkü read fonksiyonu benim ürettiğim BMPBaslik türünü tanımayacak onu
            tanıdığı char olarak görmesini sağladım.
        */
        dosya.read(reinterpret_cast<char*>(&baslik), sizeof(BmpBaslik));
        if (baslik.file_type != 0x4D42) return false; // 0x4D42 ASCII kodunda B ve M harflerine denk gelir. Dosyanın BMP olup olmadığı kontrol ediliyor.

        /*
            Bazı BMP dosyalarında başlık ile piksel verisi arasında boşluk olabiliyor.
            o aradaki veriyi kaybetmemek için ekVeri vektörü kullandım buraya yedekledim.
            Bu sayede dosya üzerine şifre yazıp dosyayı tekrar kaydettiğimde bozulma yaşamıyorum.
        */
        long boslukBoyutu = baslik.offset_data - sizeof(BmpBaslik);
        if (boslukBoyutu > 0) {
            ekVeri.resize(boslukBoyutu);
            dosya.read(reinterpret_cast<char*>(ekVeri.data()), boslukBoyutu);
        } else {
            ekVeri.clear();
        }

        uint32_t veriBoyutu = baslik.size_image;
        if (veriBoyutu == 0) veriBoyutu = baslik.file_size - baslik.offset_data;

        dosya.seekg(baslik.offset_data, ios::beg);  //offset_data kaç veri öteleme var, ios::beg başlangıç adresi
        pikselVerisi.resize(veriBoyutu);
        dosya.read(reinterpret_cast<char*>(pikselVerisi.data()), veriBoyutu);
        dosya.close();
        return true;
    }

    void dosyayiKaydet() {
        /*
            ios::trunc → eski dosyanın içi boşaltıldı, ios::binary dosya tipinin resim olduğu vurgulandı, ios::out dosyaya yazılacağı belirtildi.
        */
        ofstream dosya(aktifDosyaIsmi, ios::binary | ios::out | ios::trunc);
        if (dosya) {
            dosya.write(reinterpret_cast<char*>(&baslik), sizeof(BmpBaslik)); // Başlık header yazıldı
            if (!ekVeri.empty()) dosya.write(reinterpret_cast<char*>(ekVeri.data()), ekVeri.size()); // boşluk varsa o yazıldı dosyayı korumak için.
            dosya.write(reinterpret_cast<char*>(pikselVerisi.data()), pikselVerisi.size());  // üzerine mesaj gizlediğimiz pikselleri döktü.
            dosya.close();
            cout << "Dosya guncellendi: " << aktifDosyaIsmi << endl;
        } else {
            cout << "Hata: Dosyaya yazilamadi!" << endl;
        }
    }

    bool gizle(string mesaj) {
        /*
            tamMesaj: Mesajın başına kodun başında bahsettiğim imzayı
            sonuna da mesajın bittiğini anlamak için \0 ekledim.
        */
        string tamMesaj = IMZA + mesaj + '\0'; 
        
        if (tamMesaj.length() * 8 > pikselVerisi.size()) {
            cout << "Hata: Mesaj cok uzun!" << endl;
            return false;
        }
        
        /*
            LSB (en anlamsız bit) mantığı
            - (pikselVerisi[sira]) & 0xFE → 0xFE maskesi ile pikselin son bitini zorla 0 yapıyorum, temizliyorum.
            - sonra karakterimin bitini oraya yazıyorum.
        */
        
        int sira = 0;
        for (char k : tamMesaj) {     // For döngüsünün böyle bir kullanımı daha varmış kodumda denemek istedim (normal for döngüsü)
            for (int i = 0; i < 8; ++i) {
                int bit = (k >> i) & 1;
                pikselVerisi[sira] = (pikselVerisi[sira] & 0xFE) | bit;
                sira++;
            }
        }
        return true;
    }

    string cozVeTemizle() {
        string cozulmusVeri = "";
        char anlikKarakter = 0;
        int bitSayaci = 0;
        int toplamOkunanBit = 0;

        for (size_t i = 0; i < pikselVerisi.size(); ++i) {
            /*
                Gizlenmiş veriyi geri çözüyorum
                - Pikselin sadece son bitini alıyorum &1
                - (sonBit << bitSayaci) ile bu biti, karakter dizilimindeki asıl yerine kaydırıyorum.
                - |= opeartörü ile parça parça gelen bitleri birleştirip orijinal ASCII karakteri elde ediyorum.
                - |= = operatörü yerine veya eşittir operatörünü eklemeli yazma için kullandık eğer = kullansaydık
                ilk biti bulurdu sonra ikinci biti de bulurdu ama ilk bitin üzerine yazardı ilk biti kaybederdik.
            */
            int sonBit = pikselVerisi[i] & 1;
            anlikKarakter |= (sonBit << bitSayaci);
            bitSayaci++;
            toplamOkunanBit++;

            if (bitSayaci == 8) {
                // Eğer okuduğumuz karakter 0 ise (mesaj bitti demek)
                if (anlikKarakter == '\0') {
                    // Buraya geldiysek bir mesaj bulduk demektir.
                    // Şimdi kontrol edelim: Bu mesaj bizim imza ile mi başlıyor?
                    
                    // İmza uzunluğu kadar kısmı kontrol et
                    if (cozulmusVeri.length() >= IMZA.length() && 
                        cozulmusVeri.substr(0, IMZA.length()) == IMZA) {
                        
                        // İmzayı siliyoruz, sadece gerçek mesajı kullanıcıya verelim
                        string gercekMesaj = cozulmusVeri.substr(IMZA.length());
                        
                        // Güvenlik: Sadece geçerli mesaj varsa pikselleri beyaza boya
                        for (int j = 0; j < toplamOkunanBit; ++j) {
                            pikselVerisi[j] = 255;
                        }
                        
                        return gercekMesaj;
                    } else {
                        // Bitiş karakteri bulduk ama imza yok. Demek ki bu rastgele bir resim.
                        return ""; 
                    }
                }
                
                cozulmusVeri += anlikKarakter;
                anlikKarakter = 0;
                bitSayaci = 0;

                // Optimizasyon: Eğer okuduğumuz veri "IMZA" ile başlamıyorsa
                // boşuna tüm resmi taramaya gerek yok, hemen çıkabiliriz.
                if (cozulmusVeri.length() == IMZA.length()) {
                    if (cozulmusVeri != IMZA) {
                        return ""; // İmza tutmadı, bu resimde mesaj yok.
                    }
                }
            }
        }
        return "";
    }

};

int main() {
    BmpIsleyici arac;
    int secim;
    string dosyaAdi, girilenMesaj;

    cout << "=== MENU ===" << endl;
    cout << "1. Mesaj Gizle" << endl;
    cout << "2. Mesaj Oku ve Yok Et (Beyaza boya)" << endl;
    cout << "Secim: ";
    cin >> secim;

    cout << "Dosya adi: ";
    cin >> dosyaAdi;

    if (!arac.dosyaAc(dosyaAdi)) {
        cout << "Hata: Dosya acilamadi!" << endl;
        return 1;
    }

    if (secim == 1) {
        /*
            Önceki seçim işleminden kalma 'Enter' (\n) karakterini tampondan siliyorum.
            Yoksa getline boş bir satır okuyup mesajı almadan geçer.
        */
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Mesajiniz: ";
        getline(cin, girilenMesaj);

        if (arac.gizle(girilenMesaj)) {
            arac.dosyayiKaydet();
            cout << "Mesaj gizlendi (Imzali)." << endl;
        }
    } else if (secim == 2) {
        string sonuc = arac.cozVeTemizle();
        
        if (!sonuc.empty()) {
            cout << "\n[+] GIZLI MESAJ BULUNDU: " << sonuc << endl;
            arac.dosyayiKaydet(); 
            cout << "Mesaj okundugu icin resimden kalici olarak silindi(beyaza boyandi)." << endl;
        } else {
            cout << "Bu resimde gizli bir mesaj yok (veya imza gecersiz)." << endl;
        }
    } else {
        cout << "Gecersiz islem." << endl;
    }

    return 0;
}
