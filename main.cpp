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
void mainMenu();

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

void listAuthorsAndBooks(auto &storage) {
    auto authors = storage.template get_all<Author>();
    for (const auto& author : authors) {
        std::cout << "Author ID: " << author.id << ", Author Name: " << author.name << "\n";
        auto books = storage.template get_all<Book>(where(c(&Book::author_id) == author.id));
        if (books.empty()) {
            std::cout << "\tNo books for this author.\n";
        } else {
            for (const auto& book : books) {
                std::cout << "\tBook ID: " << book.id << ", Book Title: " << book.title
                          << (book.is_borrowed ? " (Borrowed)" : " (Available)") << "\n";
            }
        }
    }
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

        // if (book.is_borrowed) {
        //     try {
        //         // Fetch the latest borrow record for the book
        //         auto borrow_records = storage.template get_all<BorrowRecord>(
        //             where(c(&BorrowRecord::book_id) == book.id),
        //             order_by(&BorrowRecord::borrow_date).desc(),
        //             limit(1) // Ensure only the latest record is retrieved
        //         );
        //
        //         // Check if any borrow records exist
        //         if (!borrow_records.empty()) {
        //             try {
        //                 const auto &borrow_record = borrow_records.front();
        //                 // Fetch borrower details
        //                 auto borrower = storage.template get<Borrower>(borrow_record.borrower_id);
        //
        //                 std::cout << "\n  Borrower Name: " << borrower.name
        //                         << "\n  Borrow Date: " << borrow_record.borrow_date.value_or("Unknown")
        //                         << "\n  Return Date: " << borrow_record.return_date.value_or("Unknown");
        //             } catch (const std::system_error &e) {
        //                 std::cerr << "Warning: " << e.what() << '\n';
        //             }
        //         } else {
        //             std::cerr << "\n  No borrow records found for book ID " << book.id << ".\n";
        //         }
        //     } catch (const std::system_error &e) {
        //         std::cerr << "\n  Warning: Borrow record or borrower details for book ID "
        //                 << book.id << " not found.\n";
        //     }
        // }


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

void listAuthors(auto &storage) {
    auto authors = storage.template get_all<Author>();
    for (const auto &author: authors) {
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
    for (const Borrower &borrower: borrowers) {
        // Display borrower details
        std::cout << "ID: " << borrower.id
                << ", Name: " << borrower.name
                << ", Email: " << borrower.email << '\n';

        try {
            // Retrieve borrow records for this borrower
            auto borrowed_books = storage.template get_all<BorrowRecord>(
                sqlite_orm::where(sqlite_orm::c(&BorrowRecord::borrower_id) == borrower.id)
            );

            // Display books borrowed by the borrower
            if (borrowed_books.empty()) {
                std::cout << "  No books borrowed.\n";
            } else {
                try {
                    for (const BorrowRecord &borrowRecord: borrowed_books) {
                        auto book = storage.template get<Book>(borrowRecord.book_id);
                        std::cout << "  Book Borrowed: " << book.title << '\n';
                    }
                } catch (std::exception &e) {
                    std::cerr << "Borrowed book has been deleted/not found: " << e.what() << '\n';
                }
            }
        } catch (std::exception &e) {
            std::cerr << "Warning: " << e.what() << '\n';
        }
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
        // std::cout << "Today's date: " << current_date << "\n";

        // Prompt the user for the return date
        // std::cout << "Enter return date (dd-mm-yyyy): ";
        // std::string return_date;
        // std::cin >> return_date;

        // Validate the return date format
        // std::regex date_regex(R"(^\d{2}-\d{2}-\d{4}$)");
        // if (!std::regex_match(return_date, date_regex)) {
            // std::cout << "Invalid date format. Please enter the date in dd-mm-yyyy format.\n";
            // return;
        // }

        // Insert borrow record into the database
        storage.insert(BorrowRecord{-1, book_id, borrower_id, current_date, {}});

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
        // Fetch all books that are currently borrowed
        auto borrowed_books = storage.template get_all<Book>(
            where(c(&Book::is_borrowed) == true)
        );

        if (borrowed_books.empty()) {
            std::cout << "No books are currently borrowed.\n";
            return;
        }

        std::vector<std::pair<BorrowRecord, Book>> records_to_return;

        // Collect all records of borrowed books
        for (const Book &book : borrowed_books) {
            try {
                auto borrow_records = storage.template get_all<BorrowRecord>(
                    where(c(&BorrowRecord::book_id) == book.id)
                );

                for (const auto &record : borrow_records) {
                    if (!record.return_date.has_value()) { // Check for NULL return_date manually
                        records_to_return.emplace_back(record, book);
                    }
                }
            } catch (const std::system_error &e) {
                std::cerr << "Warning: No borrow record found for book ID " << book.id << ".\n";
            }
        }

        if (records_to_return.empty()) {
            std::cout << "No unreturned borrow records found.\n";
            return;
        }

        // Display the list of unreturned borrow records
        std::cout << "Borrow Records:\n";
        for (const auto &[record, book] : records_to_return) {
            try {
                auto borrower = storage.template get<Borrower>(record.borrower_id);

                std::cout << "Borrow ID: " << record.id
                          << " | Book: " << book.title
                          << " | Borrower: " << borrower.name
                          << " | Borrow Date: " << record.borrow_date.value_or("Unknown") << "\n";
            } catch (const std::system_error &e) {
                std::cerr << "Warning: Borrower details not found for record ID " << record.id << ".\n";
            }
        }

        // Prompt the user to select a borrow record to return
        int borrow_id;
        std::cout << "\nEnter the Borrow ID to mark as returned: ";
        std::cin >> borrow_id;

        // Find the selected borrow record
        auto it = std::find_if(records_to_return.begin(), records_to_return.end(),
                               [borrow_id](const std::pair<BorrowRecord, Book> &entry) {
                                   return entry.first.id == borrow_id;
                               });

        if (it == records_to_return.end()) {
            std::cerr << "Error: Invalid Borrow ID entered.\n";
            return;
        }

        BorrowRecord record = it->first;
        Book book = it->second;

        // Get the current date in DD-MM-YYYY format
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        std::tm local_time;
#ifdef _WIN32
        localtime_s(&local_time, &time_t_now);
#else
        localtime_r(&time_t_now, &local_time);
#endif
        std::ostringstream current_date_stream;
        current_date_stream << std::put_time(&local_time, "%d-%m-%Y");
        std::string current_date = current_date_stream.str();

        // Update the borrow record with the return date
        record.return_date = current_date;
        storage.update(record);

        // Mark the book as available
        book.is_borrowed = false;
        storage.update(book);

        std::cout << "Book '" << book.title << "' has been successfully returned on " << current_date << ".\n";

    } catch (const std::exception &e) {
        std::cerr << "Custom Error: " << e.what() << '\n';
    }
}

void removeAuthor(auto &storage) {
    try {
        // List all authors and their books
        listAuthorsAndBooks(storage);

        std::cout << "Enter Author ID to delete: ";
        int author_id;
        std::cin >> author_id;

        // First, check if there are any borrowed books by this author
        auto books = storage.template get_all<Book>(where(c(&Book::author_id) == author_id));
        bool hasBorrowedBooks = false;

        // Check if any of the books are borrowed
        for (const auto& book : books) {
            if (book.is_borrowed) {
                hasBorrowedBooks = true;
                break;
            }
        }

        if (hasBorrowedBooks) {
            std::cout << "Cannot delete the author because some of their books are borrowed.\n";
            return;
        }

        // Delete borrow records for books by this author
        for (const auto& book : books) {
            // Get borrow records related to this book
            auto borrow_records = storage.template get_all<BorrowRecord>(where(c(&BorrowRecord::book_id) == book.id));
            // Delete each borrow record
            for (const auto& record : borrow_records) {
                storage.template remove<BorrowRecord>(record.id);
                std::cout << "Removed BorrowRecord ID: " << record.id << "\n";
            }
            // Remove the book itself
            storage.template remove<Book>(book.id);
            std::cout << "Removed Book ID: " << book.id << "\n";
        }

        // Finally, remove the author
        storage.template remove<Author>(author_id);
        std::cout << "Author and their books have been deleted successfully.\n";

    } catch (const std::exception &e) {
        std::cerr << "Custom Error: " << e.what() << '\n';
    }
}

void removeBook(auto &storage) {
    try {
        // List all books
        listBooks(storage);
        std::cout << "Enter book ID to delete: ";
        int book_id;
        std::cin >> book_id;
        // First, remove related borrow records
        auto borrow_records = storage.template get_all<BorrowRecord>(
            where(c(&BorrowRecord::book_id) == book_id)
        );
        // Delete each borrow record related to this book
        for (const BorrowRecord &record : borrow_records) {
            storage.template remove<BorrowRecord>(record.id);
            std::cout << "Removed BorrowRecord ID: " << record.id << "\n";
        }
        // Now, remove the book itself
        storage.template remove<Book>(book_id);
        std::cout << "Book deleted successfully.\n";

    } catch (const std::exception &e) {
        std::cerr << "Custom Error: " << e.what() << '\n';
    }
}

void showBorrowRecords(auto &storage) {
    try {
        auto borrow_records = storage.template get_all<BorrowRecord>();
        for (const BorrowRecord &record : borrow_records) {
            Book book = storage.template get<Book>(record.book_id);
            Borrower borrower_profile = storage.template get<Borrower>(record.borrower_id);
            std::cout << "Borrow ID: " << record.id << " || Book: " << book.title << " || Borrower Name: " <<
                borrower_profile.name << " || Borrowed Date: " << record.borrow_date.value_or("Unknown") <<
                    " || Return Date: " << record.return_date.value_or("N/A") << "\n";
        }

    } catch (std::exception &e) {
        std::cerr << "Custom Error: " << e.what() << '\n';
    }
}

void showMain() {
    std::cout << "\n\nLibrary Management System\n";
    std::cout << "1. Manage Books\n";
    std::cout << "2. Manage Authors\n";
    std::cout << "3. Manage Borrowers\n";
    std::cout << "4. Borrow and Return Books\n";
    std::cout << "5. Import/Export Books\n";
    std::cout << "0. Exit\n";
}

void bookMenu() {
    std::cout << "\n--- Manage Books ---\n";
    std::cout << "1. Add Book\n";
    std::cout << "2. Remove Book\n";
    std::cout << "3. List Books\n";
    std::cout << "4. Update Book\n";
    std::cout << "0. Back to Main Menu\n";
}

void authorMenu() {
    std::cout << "\n--- Manage Authors ---\n";
    std::cout << "1. Add Author\n";
    std::cout << "2. List Authors\n";
    std::cout << "3. Delete Authors\n";
    std::cout << "0. Back to Main Menu\n";
}

void borrowerMenu() {
    std::cout << "\n--- Manage Borrowers ---\n";
    std::cout << "1. Register Borrower\n";
    std::cout << "2. List Borrowers\n";
    std::cout << "0. Back to Main Menu\n";
}

void borrowReturnMenu() {
    std::cout << "\n--- Borrow and Return Books ---\n";
    std::cout << "1. Borrow Book\n";
    std::cout << "2. Return Book\n";
    std::cout << "3. Borrow Records\n";
    std::cout << "0. Back to Main Menu\n";
}

void handleBookMenu(auto& storage) {
    int choice;
    while (true) {
        bookMenu();
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
            case 0:
                return;
            default:
                std::cout << "Invalid choice.\n";
                break;
        }
    }
}

void handleAuthorMenu(auto& storage) {
    int choice;
    while (true) {
        authorMenu();
        std::cout << "Enter choice: ";
        std::cin >> choice;
        std::cin.ignore();
        std::cout << "\n---------\n";

        switch (choice) {
            case 1:
                addAuthor(storage);
                break;
            case 2:
                listAuthorsAndBooks(storage);
                break;
            case 3:
                removeAuthor(storage);
                break;
            case 0:
                return;
            default:
                std::cout << "Invalid choice.\n";
                break;
        }
    }
}

void handleBorrowerMenu(auto& storage) {
    int choice;
    while (true) {
        borrowerMenu();
        std::cout << "Enter choice: ";
        std::cin >> choice;
        std::cin.ignore();
        std::cout << "\n---------\n";

        switch (choice) {
            case 1:
                registerBorrower(storage);
                break;
            case 2:
                listBorrowers(storage);
                break;
            case 0:
                return;
            default:
                std::cout << "Invalid choice.\n";
                break;
        }
    }
}

void handleBorrowReturnMenu(auto& storage) {
    int choice;
    while (true) {
        borrowReturnMenu();
        std::cout << "Enter choice: ";
        std::cin >> choice;
        std::cin.ignore();
        std::cout << "\n---------\n";

        switch (choice) {
            case 1:
                borrowBook(storage);
                break;
            case 2:
                returnBook(storage);
                break;
            case 3:
                showBorrowRecords(storage);
                break;
            case 0:
                return;
            default:
                std::cout << "Invalid choice.\n";
                break;
        }
    }
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

    while (true) {
        showMain();
        int choice;
        std::cout << "Enter choice: ";
        std::cin >> choice;
        std::cin.ignore();
        std::cout << "\n---------\n";

        switch (choice) {
            case 1:
                handleBookMenu(storage);
                break;
            case 2:
                handleAuthorMenu(storage);
                break;
            case 3:
                handleBorrowerMenu(storage);
                break;
            case 4:
                handleBorrowReturnMenu(storage);
                break;
            case 0:
                std::cout << "Exiting the program. Goodbye!\n";
                return 0;
            default:
                std::cout << "Invalid choice.\n";
                break;
        }
        std::string empty_line = "---------";
        std::cout << "\nPress enter to continue:";
        std::getline(std::cin, empty_line);
    }
}