#include "sqlite_orm/sqlite_orm.h"
#include <iostream>
#include <string>
#include <optional>
#include <chrono>
#include <fstream>
#include <sstream>
#include <ctime>
#include <regex>

using namespace sqlite_orm;

// Entities
struct Book {
    int id;
    std::string title;
    int author_id;
    std::string genre;
    bool is_borrowed;
};

struct Author {
    int id;
    std::string name;
};

struct Borrower {
    int id;
    std::string name;
    std::string email;
};

struct BorrowRecord {
    int id;
    int book_id;
    int borrower_id;
    std::optional<std::string> borrow_date;
    std::optional<std::string> return_date;
};


// prototypes
auto createStorage();
void createTestData(auto &storage);
void importBooksFromFile(const std::string &file_path, auto &storage);
void exportBooksToFile(const std::string &file_path, auto &storage);
void addBook(auto &storage);
void updateBook(auto &storage);
void listBooks(auto &storage);
void addAuthor(auto &storage);
void listAuthorsAndBooks(auto &storage);
void listAuthors(auto &storage);
void registerBorrower(auto &storage);
void listBorrowers(auto &storage);
void borrowBook(auto &storage);
void returnBook(auto &storage);
void removeBook(auto &storage);

// Storage setup
auto createStorage() {
    using namespace sqlite_orm;

    return make_storage("library.sqlite",
                        make_table("books",
                                   make_column("id", &Book::id, primary_key().autoincrement()),
                                   make_column("title", &Book::title),
                                   make_column("author_id", &Book::author_id),
                                   make_column("genre", &Book::genre),
                                   make_column("is_borrowed", &Book::is_borrowed)),
                        make_table("authors",
                                   make_column("id", &Author::id, primary_key().autoincrement()),
                                   make_column("name", &Author::name)),
                        make_table("borrowers",
                                   make_column("id", &Borrower::id, primary_key().autoincrement()),
                                   make_column("name", &Borrower::name),
                                   make_column("email", &Borrower::email)),
                        make_table("borrow_records",
                                   make_column("id", &BorrowRecord::id, primary_key().autoincrement()),
                                   make_column("book_id", &BorrowRecord::book_id),
                                   make_column("borrower_id", &BorrowRecord::borrower_id),
                                   make_column("borrow_date", &BorrowRecord::borrow_date),
                                   make_column("return_date", &BorrowRecord::return_date)));
}
void createTestData(auto &storage) {
    // Add authors
    storage.replace(Author{-1, "J.K. Rowling"});
    storage.replace(Author{-1, "George Orwell"});
    storage.replace(Author{-1, "J.R.R. Tolkien"});

    // Add books
    storage.replace(Book{-1, "Harry Potter", 1, "Fantasy", false});
    storage.replace(Book{-1, "1984", 2, "Dystopian", false});
    storage.replace(Book{-1, "The Hobbit", 3, "Fantasy", false});

    // Add borrowers
    storage.replace(Borrower{-1, "Alice Smith", "alice@example.com"});
    storage.replace(Borrower{-1, "Bob Johnson", "bob@example.com"});

    // Add borrow records
    storage.replace(BorrowRecord{-1, 1, 1, "2024-11-01", "2024-11-10"});
    storage.replace(BorrowRecord{-1, 2, 2, "2024-11-05", "2024-11-15"});
}
void importBooksFromFile(const std::string &file_path, auto &storage) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << file_path << "\n";
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream stream(line);
        std::string title, genre;
        int author_id;

        // Parse the line
        std::getline(stream, title, ',');
        stream >> author_id;
        stream.ignore(); // Skip the comma
        std::getline(stream, genre, ',');

        // Trim whitespace
        title.erase(title.find_last_not_of(" \t\n\r") + 1);
        genre.erase(genre.find_last_not_of(" \t\n\r") + 1);

        // Insert into the database
        Book book = {0, title, author_id, genre}; // Assuming `id` is autoincrement
        storage.insert(book);
        std::cout << "Inserted book: " << title << "\n";
    }

    file.close();
    std::cout << "Finished importing books from file.\n";
}
void exportBooksToFile(const std::string &file_path, auto &storage) {
    std::ofstream file(file_path);
    if (!file.is_open()) {
        std::cerr << "Failed to create file: " << file_path << "\n";
        return;
    }

    // Fetch all books
    auto books = storage.template get_all<Book>();
    file << "book_id,book_name,author_name,borrowed/available,borrow_date,return_date,borrower_name\n";

    for (const Book &book: books) {
        // Fetch author information
        std::optional<Author> author = storage.template get_optional<Author>(book.author_id);
        std::string author_name = author ? author->name : "Unknown";

        // Initialize borrow-related data
        std::string status = "available";
        std::string borrow_date = "N/A";
        std::string return_date = "N/A";
        std::string borrower_name = "N/A";

        if (book.is_borrowed) {
            status = "borrowed";

            // Fetch borrow record
            if (std::optional<BorrowRecord> borrower_data = storage.template get_optional<BorrowRecord>(book.id)) {
                borrow_date = borrower_data->borrow_date.value_or("N/A");
                return_date = borrower_data->return_date.value_or("N/A");

                // Fetch borrower details
                std::optional<Borrower> borrower_details = storage.template get_optional<Borrower>(
                    borrower_data->borrower_id);
                borrower_name = borrower_details ? borrower_details->name : "Unknown";
            }
        }
        std::cout << book.id << ","
                << book.title << ","
                << author_name << ","
                << status << ","
                << borrow_date << ","
                << return_date << ","
                << borrower_name << "\n";
        // Write to file
        file << book.id << ","
                << book.title << ","
                << author_name << ","
                << status << ","
                << borrow_date << ","
                << return_date << ","
                << borrower_name << "\n";
    }

    file.close();
    std::cout << "Books exported to file: " << file_path << "\n";
}
void addBook(auto &storage) {
    std::string title, genre;
    int author_id;

    std::cout << "Enter book title: ";
    std::getline(std::cin, title);
    // list authors
    listAuthors(storage);
    std::cout << "Enter author ID: ";
    std::cin >> author_id;

    std::cin.ignore(); // Clear input buffer
    std::cout << "Enter genre: ";
    std::getline(std::cin, genre);

    storage.insert(Book{-1, title, author_id, genre, false});
    std::cout << "Book added successfully.\n";
}
void updateBook(auto &storage) {
    try {
        int book_id;
        std::cout << "Enter book ID: ";
        std::cin >> book_id;
        std::cin.ignore(); // Clear the input buffer

        // Find the book by ID
        auto book = storage.template get_optional<Book>(book_id);
        if (!book) {
            std::cout << "Book with ID " << book_id << " not found.\n";
            return;
        }

        // Display current book details
        std::cout << "Book Found > ID: " << book->id
                << ", Title: " << book->title
                << ", Author ID: " << book->author_id
                << ", Genre: " << book->genre << "\n";

        // Get updated details from the user
        std::string new_title, new_genre;
        int new_author_id;

        std::cout << "Enter new book title (leave blank to keep current): ";
        std::getline(std::cin, new_title);
        if (!new_title.empty()) {
            book->title = new_title;
        }

        std::cout << "Enter new author ID (or -1 to keep current): ";
        std::cin >> new_author_id;
        std::cin.ignore(); // Clear the input buffer
        if (new_author_id != -1) {
            book->author_id = new_author_id;
        }

        std::cout << "Enter new genre (leave blank to keep current): ";
        std::getline(std::cin, new_genre);
        if (!new_genre.empty()) {
            book->genre = new_genre;
        }

        // Update the book in the database
        storage.update(*book);

        std::cout << "Book updated successfully!\n";
    } catch (const std::exception &e) {
        std::cerr << "Custom Error: " << e.what() << '\n';
    }
}
void listBooks(auto &storage) {
    auto books = storage.template get_all<Book>();
    for (const auto &book: books) {
        std::optional<Author> author;
        try {
            author = storage.template get<Author>(book.author_id);
        } catch (const std::system_error &e) {
            std::cerr << "Warning: Author with ID " << book.author_id << " not found.\n";
        }

        std::cout << "ID: " << book.id
                << ", Title: " << book.title
                << ", Author: " << (author ? author->name : "Unknown")
                << ", Genre: " << book.genre
                << ", Borrowed: " << (book.is_borrowed ? "Yes" : "No");

        if (book.is_borrowed) {
            try {
                // Fetch the borrow record for the book
                auto borrow_record = storage.template get<BorrowRecord>(book.id);

                // Fetch borrower details
                auto borrower = storage.template get<Borrower>(borrow_record.borrower_id);

                std::cout << "\n  Borrower Name: " << borrower.name
                        << "\n  Borrow Date: " << borrow_record.borrow_date.value_or("Unknown")
                        << "\n  Return Date: " << borrow_record.return_date.value_or("Unknown");
            } catch (const std::system_error &e) {
                std::cerr << "\n  Warning: Borrow record or borrower details for book ID "
                        << book.id << " not found.\n";
            }
        }

        std::cout << '\n';
    }
}
void addAuthor(auto &storage) {
    std::string name;
    std::cout << "Enter author name: ";
    std::getline(std::cin, name);
    storage.insert(Author{-1, name});
    std::cout << "Author added successfully.\n";
}
void listAuthorsAndBooks(auto &storage) {
    auto authors = storage.template get_all<Author>();
    for (const auto &author : authors) {
        std::cout << "ID: " << author.id << ", Name: " << author.name << '\n';
        // Fetch books written by the author
        auto booksByAuthor = storage.template get_all<Book>(where(c(&Book::author_id) == author.id));

        if (booksByAuthor.empty()) {
            std::cout << "   No books written by this author.\n";
        } else {
            std::cout << "   Books:\n";
            for (const auto &book : booksByAuthor) {
                std::cout << "      - " << book.title << '\n';
            }
        }
    }
}
void listAuthors(auto &storage) {
    auto authors = storage.template get_all<Author>();
    for (const auto &author : authors) {
        std::cout << "ID: " << author.id << ", Name: " << author.name << '\n';
    }
}
void registerBorrower(auto &storage) {
    std::string name, email;
    std::cout << "Enter borrower name: ";
    std::getline(std::cin, name);
    std::cout << "Enter borrower email: ";
    std::getline(std::cin, email);

    storage.insert(Borrower{-1, name, email});
    std::cout << "Borrower registered successfully.\n";
}
void listBorrowers(auto &storage) {
    auto borrowers = storage.template get_all<Borrower>();
    for (const auto &borrower: borrowers) {
        std::cout << "ID: " << borrower.id << ", Name: " << borrower.name
                << ", Email: " << borrower.email << '\n';
    }
}
void borrowBook(auto &storage) {
    try {
        int book_id, borrower_id;
        std::cout << "Enter book ID: ";
        std::cin >> book_id;

        auto book = storage.template get<Book>(book_id);
        if (book.is_borrowed) {
            std::cout << "Book is already borrowed.\n";
            return;
        }

        std::cout << "Enter borrower ID: ";
        std::cin >> borrower_id;

        // Get current date
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        std::tm local_time;
#ifdef _WIN32
        localtime_s(&local_time, &time_t_now);
#else
    localtime_r(&time_t_now, &local_time);
#endif

        // Format current date as DD-MM-YYYY
        std::ostringstream current_date_stream;
        current_date_stream << std::put_time(&local_time, "%d-%m-%Y");
        std::string current_date = current_date_stream.str();

        // Display the current date to the user
        std::cout << "Today's date: " << current_date << "\n";

        // Prompt the user for the return date
        std::cout << "Enter return date (dd-mm-yyyy): ";
        std::string return_date;
        std::cin >> return_date;

        // Validate the return date format
        std::regex date_regex(R"(^\d{2}-\d{2}-\d{4}$)");
        if (!std::regex_match(return_date, date_regex)) {
            std::cout << "Invalid date format. Please enter the date in dd-mm-yyyy format.\n";
            return;
        }

        // Insert borrow record into the database
        storage.insert(BorrowRecord{-1, book_id, borrower_id, current_date, return_date});

        // Mark the book as borrowed and update the database
        book.is_borrowed = true;
        storage.update(book);

        std::cout << "Book borrowed successfully.\n";
    } catch (const std::exception &e) {
        std::cerr << "Custom Error: " << e.what() << '\n';
    }
}
void returnBook(auto &storage) {
    try {
        auto all_records = storage.template get_all<BorrowRecord>();
        for (const BorrowRecord &record: all_records) {
            std::cout << "ID: " << record.id << ", Name: " << record.book_id
                    << ", Date: " << record.borrow_date.value_or("Unknown")
                    << "\n  Return Date: " << record.return_date.value_or("Unknown")
                    << ", Borrower ID: " << record.borrower_id << '\n';
        }

        int record_id;
        std::cout << "Enter borrow record ID: ";
        std::cin >> record_id;

        BorrowRecord record = storage.template get<BorrowRecord>(record_id);
        auto book = storage.template get<Book>(record.book_id);
        book.is_borrowed = false;
        storage.template remove<BorrowRecord>(record.id);
        storage.update(book);
        std::cout << "Book returned successfully.\n";
    } catch (const std::exception &e) {
        std::cerr << "Custom Error: " << e.what() << '\n';
    }
}
void removeBook(auto &storage) {
    // List all books
    try {
        listBooks(storage);
        std::cout << "Enter book ID: ";
        int book_id;
        std::cin >> book_id;
        auto book = storage.template get<Book>(book_id);
        storage.template remove<Book>(book_id);
        std::cout << "Book deleted successfully.\n";
    } catch (const std::exception &e) {
        std::cerr << "Custom Error: " << e.what() << '\n';
    }
}

void menu() {
    std::cout << "\n\nLibrary Management System\n";
    std::cout << "1. Add Book\n";
    std::cout << "2. Remove Book\n";
    std::cout << "3. List Books\n";
    std::cout << "4. Update Book\n";
    std::cout << "5. Add Author\n";
    std::cout << "6. List Authors\n";
    std::cout << "7. Register Borrower\n";
    std::cout << "8. List Borrowers\n";
    std::cout << "9. Borrow Book\n";
    std::cout << "10. Return Book\n";
    std::cout << "11. Import Books\n";
    std::cout << "12. Export Books\n";
}

int main() {
    auto storage = createStorage();
    try {
        storage.sync_schema();
        std::cout << "Database schema created successfully.\n";
        std::cout << "To use this application first create authors and then start adding books" << std::endl;
        std::cout << "Register Borrowers to use borrow and return features" << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "Custom Error: " << e.what() << '\n';
    }
    // Create test data
    // createTestData(storage);

    while (true) {
        menu();
        int choice;
        std::cout << "Enter choice: ";
        std::cin >> choice;
        std::cin.ignore();
        std::cout << "\n---------\n";

        switch (choice) {
            case 1:
                addBook(storage);
            break;
            case 2:
                removeBook(storage);
            break;
            case 3:
                listBooks(storage);
            break;
            case 4:
                updateBook(storage);
            break;
            case 5:
                addAuthor(storage);
            break;
            case 6:
                listAuthorsAndBooks(storage);
            break;
            case 7:
                registerBorrower(storage);
            break;
            case 8:
                listBorrowers(storage);
            break;
            case 9:
                borrowBook(storage);
            break;
            case 10:
                returnBook(storage);
            break;
            case 11:
                importBooksFromFile("C:/Users/mrbil/CLionProjects/librarymanagement/books.txt", storage);
            break;
            case 12:
                exportBooksToFile("export_book.txt", storage);
            break;
            case 0:
                std::cout << "Exiting the program. Goodbye!\n";
            return 0;
            default:
                std::cout << "Invalid choice.\n";
            break;
        }

    }
}
