#include <nexus.h>

int main(int argc, char* argv[]) {

	qDebug() << "SailPalServer: main() started";
	
	try {
		qDebug() << "SailPalServer: Creating ConsoleApplication...";
		ConsoleApplication app(argc, argv);
		qDebug() << "SailPalServer: ConsoleApplication created successfully";
		
		qDebug() << "SailPalServer: Calling app.start()...";
		int result = app.start();
		qDebug() << "SailPalServer: app.start() returned:" << result;
		return result;
	}
	catch (const Error& err) {
		qCritical() << "SailPalServer: Caught Error exception:" << err.message();
		Logger::instance().logError(err);
		return EXIT_FAILURE;
	}
	catch (const std::exception& ex) {
		qCritical() << "SailPalServer: Caught std::exception:" << ex.what();
		Logger::Logger::instance().logException(ex);
		return EXIT_FAILURE;
	}
	catch (...) {
		qCritical() << "SailPalServer: Caught unknown exception!";
		Logger::Logger::instance().logFatal("An unknown error occurred!");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}