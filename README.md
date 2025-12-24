# BEHAChat: Modern C++ ve gRPC ile Yüksek Performanslı Sohbet Mimarisi
![Language](https://img.shields.io/badge/dil-C%2B%2B-blue.svg?style=flat&logo=c%2B%2B)
![Framework](https://img.shields.io/badge/framework-gRPC-green.svg?style=flat&logo=google)
![Build](https://img.shields.io/badge/build-CMake-orange.svg?style=flat&logo=cmake)
![Platform](https://img.shields.io/badge/platform-Linux-lightgrey.svg?style=flat&logo=linux)
![Database](https://img.shields.io/badge/database-PostgreSQL-336791.svg?style=flat&logo=postgresql&logoColor=white)

**BEHAChat**, **Modern C++ (C++20)** standartları kullanılarak geliştirilmiş; ölçeklenebilir, thread-safe (iş parçacığı güvenli) ve mikroservis mimarisine uygun bir mesajlaşma sistemidir.

 **Kimlik Doğrulama (Authentication)** işlemleri güvenli bir gRPC servisi üzerinden yürütülürken, gerçek zamanlı mesajlaşma trafiği **TCP Socket** (Asenkron I/O) mimarisi üzerinden yönetilir.

---

## 🚀 Projenin Teknik Özellikleri


* **Modern C++20 Standartları:** `std::optional`, `std::chrono`, `Smart Pointers (RAII)` ve `Structured Bindings` gibi modern teknikler.
* **Hibrit İletişim Mimarisi:**
    * **Auth Servisi:** Güvenli ve yapılandırılmış veri transferi için **gRPC (Protocol Buffers)**.
    * **Mesajlaşma:** Düşük gecikme (Low Latency) için saf **TCP Socket Programlama**.
* **Thread-Safe Concurrency:**
    * Çoklu istemci bağlantılarını yönetmek için `std::mutex` ve `std::lock_guard` ile korunan kritik bölümler.
* **O(1) Karmaşıklıkta Oturum Yönetimi:**
    * Özel olarak tasarlanmış `TokenManager` sınıfı, kullanıcı oturumlarını `std::unordered_map` üzerinde yöneterek anlık erişim sağlar. (Bazı fonksiyonlar bu durumu ihlal ediyor, bu durum çözülecektir.)
* **Heartbeat & Aktivite Takibi:**
    * Kullanıcıların çevrimiçi durumlarını (Online/Idle/Offline) son işlem zamanına göre dinamik hesaplayan zaman damgası mekanizması.

---

## 🛠️ Teknoloji Yığını

| Bileşen | Teknoloji | Kullanım Amacı |
| :--- | :--- | :--- |
| **Dil** | C++20 | Ana geliştirme dili |
| **Build Sistemi** | CMake (3.15+) | Derleme ve bağımlılık yönetimi |
| **RPC Framework** | gRPC & Protobuf | Servisler arası iletişim ve Auth |
| **Ağ Protokolü** | TCP / POSIX Sockets | Gerçek zamanlı mesajlaşma |
| **Veritabanı** | PostgreSQL | Kullanıcı verilerinin kalıcılığı (Entegre) |
| **Platform** | Linux (Ubuntu/Debian) | Hedef işletim sistemi |


## 🏗️ Kurulum ve Derleme Adımları

Bu proje Linux (Ubuntu/Debian) ortamında geliştirilmiştir.

### 1. Gereksinimler
**Sisteminizi derlemek için aşağıdaki kütüphanelerin ve araçların kurulu olması gerekir:**

* `g++` (C++20 desteği ile)
* `cmake` (v3.15+)
* `grpc` ve `protobuf` kütüphaneleri ve derleyicileri
* `libpqxx` (PostgreSQL C++ sürücüsü)

```bash
**Ubuntu için tek satırda kurulum:**


sudo apt update
sudo apt install build-essential cmake libgrpc++-dev libprotobuf-dev protobuf-compiler-grpc libpqxx-dev
```



### 2. Projeyi Derleme (Server)

**Repoyu temiz tutmak için "Out-of-Source Build" yöntemi kullanılır. Aşağıdaki adımları sırasıyla uygulayın:**
```bash
Repoyu klonlayın ve dizine girin:

git clone https://github.com/hakanbarin/BEHAChat.git
cd BEHAChat


Build klasörü oluşturun:

mkdir build && cd build


CMake ile konfigüre edin ve derleyin:

cmake ..
make -j4
```



### 3. Konfigürasyon (Ortam Değişkenleri)

**Sunucu başlatılmadan önce veritabanı ve port ayarlarını **kendi sisteminize göre** düzenleyin.**

```bash
Sunucunun dinleyeceği port (Örn: 8080 veya 9090)

export CHAT_PORT=8080


Admin işlemleri için belirlediğiniz şifre

export ADMIN_PASS="buraya_zor_bir_sifre_yazin"


PostgreSQL Bağlantı Bilgileri (DİKKAT: Kendi kurulumunuza göre düzenleyin!)
dbname : Veritabanı adınız (Varsayılan: postgres)
user : Veritabanı kullanıcısı (Varsayılan: postgres)
password : PostgreSQL kurulumunda belirlediğiniz şifre

export DATABASE_URL="dbname=postgres user=postgres password=SENIN_GERCEK_SIFREN host=localhost" 
```



### 4. Sunucuyu Başlatma

**Derleme ve konfigürasyon tamamlandıktan sonra sunucuyu aşağıdaki komutla başlatabilirsiniz:**
 ```bash
./chat_server
```

### 5. İstemciyi (Client) Derleme ve Başlatma

**İstemci uygulaması client/ klasöründe ayrı bir proje olarak bulunur. Server çalışırken yeni bir terminal açıp aşağıdaki adımları uygulayın:**

```bash
Yeni bir terminal açın ve proje dizinine gidin:

cd BEHAChat/client



Build klasörü oluşturun ve derleyin:

mkdir -p build && cd build
cmake .. && make



İstemciyi başlatın:

./chat_client

```
### 🧩 Mimari Detaylar

TokenManager (Oturum Yönetimi)

Sistemdeki en kritik bileşenlerden biridir. Kullanıcı giriş yaptığında (Login), TokenManager benzersiz bir oturum anahtarı üretir.

    Encapsulation: Kullanıcı verileri (Yetki, Username, Son Aktivite) UserInfo sınıfı içinde kapsüllenmiştir.

    Performans: Tüm sorgulamalar Hash Map tabanlı olduğu için kullanıcı sayısı artsa bile erişim hızı sabittir (O(1)). (Bazı yerlerde O(1) olmayan fonksiyonlar var bunlar düzeltilecektir)

Gelecek Planları 

    [ ] Projenin web üzerine taşınması.

    [ ] Mesaj geçmişinin veritabanında saklanması ve asenkron yüklenmesi.

    [ ] Özel mesajlaşma ve oda desteği.