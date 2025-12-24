// ═══════════════════════════════════════════════════════════════════════════
//                         SECURE CHAT CLIENT - MAIN
// Qt6 + gRPC tabanlı chat istemcisi giriş noktası
// ═══════════════════════════════════════════════════════════════════════════

#include <QApplication>
#include "MainWindow.hpp"

int main(int argc, char *argv[])
{
    // Qt uygulaması oluştur
    QApplication app(argc, argv);
    
    // Uygulama bilgilerini ayarla
    app.setApplicationName("Secure Chat Client");
    app.setApplicationVersion("2.0");
    app.setOrganizationName("SecureChat");
    
    // Ana pencereyi oluştur ve göster
    MainWindow mainWindow;
    mainWindow.show();
    
    // Uygulama döngüsünü başlat
    return app.exec();
}
