# FTP Server Project

## 📌 Overview
This FTP Server project implements a basic File Transfer Protocol (FTP) system, allowing users to authenticate, upload, download, and manage files on a remote server.

## 🚀 Features
- **User Authentication**: Supports login and access control.
- **File Upload (STOR)**: Users can upload files after successful authentication.
- **File Download (RETR)**: Retrieve files from the server.
- **Directory Navigation (CWD, PWD, LIST)**: Navigate directories and list files.
- **Error Handling**: Invalid login attempts, nonexistent directories, and file permission issues handled gracefully.
- **Multiple Client Support**: Handles simultaneous connections.

## 🛠 Installation & Setup

### Prerequisites
- Linux/MacOS (or WSL on Windows)
- GCC Compiler
- Make utility

### Steps to Run
1. **Clone the repository**:
   ```sh
   git clone https://github.com/Zaid141/FTP.git
   cd FTP
   ```
2. **Compile the project**:
   ```sh
   make
   ```
3. **Start the FTP Server**:
   ```sh
   cd server
   ./server
   ```
4. **Start the FTP Client**:
   ```sh
   cd client
   ./client
   ```

## 📂 Command Tests

### Login Tests
| **Description**                          | **Input**           | **Client Output**        | **Server Output**         |
|------------------------------------------|---------------------|--------------------------|---------------------------|
| Normal login                             | `USER bob` `PASS donuts` | User logged in | Successful login |
| Invalid username                         | `USER sam` `PASS donuts` | Not logged in | Username verification failed |
| Invalid password                         | `USER bob` `PASS cake` | Not logged in | Password incorrect |

### File Transfer Tests
| **Description**                          | **Input**           | **Client Output**        | **Server Output**         |
|------------------------------------------|---------------------|--------------------------|---------------------------|
| Upload file after login                  | `STOR beasts.pdf`   | Transfer complete        | File stored successfully |
| Upload file without login                | `STOR beasts.pdf`   | Not logged in            | Access denied |
| Download existing file                   | `RETR friends.pdf`  | Transfer complete        | File sent successfully |
| Download nonexistent file                | `RETR missing.txt`  | No such file             | File not found |

### Directory Navigation Tests
| **Description**                          | **Input**           | **Client Output**        | **Server Output**         |
|------------------------------------------|---------------------|--------------------------|---------------------------|
| Change to valid directory                | `CWD images`        | Directory changed        | Successfully changed |
| Change to invalid directory              | `CWD images123`     | No such directory        | Directory change failed |
| List files in directory                  | `LIST`              | File list displayed      | Directory listing sent |

## 👥 Contributors
- **Zaid Shahid** - *Developer*

## 📜 License
This project is licensed under the MIT License.
