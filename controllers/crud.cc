#include "crud.h"
#include <drogon/HttpAppFramework.h>
#include <inja/inja.hpp>
#include <sqlite3.h>
#include <iostream>

using namespace drogon;
using namespace inja;

sqlite3* openDB() {
    sqlite3* db;
    if (sqlite3_open("../items.db", &db) != SQLITE_OK) {
        std::cerr << "Can't open DB: " << sqlite3_errmsg(db) << std::endl;
        return nullptr;
    }
    return db;
}

void Crud::index(const HttpRequestPtr &req,
                 std::function<void(const HttpResponsePtr &)> &&callback) {
    nlohmann::json Title; 
    Title["title"] = "Test 2";          
    inja::Environment env;
    // Just load the HTML with an empty table
    auto tpl = env.parse_template("../views/table.html");

    // No data passed yet
    std::string result = env.render(tpl, Title);

    auto resp = HttpResponse::newHttpResponse();
    resp->setContentTypeCode(CT_TEXT_HTML);
    resp->setBody(result);
    callback(resp);
}

void Crud::createPage(const HttpRequestPtr &req,
                 std::function<void(const HttpResponsePtr &)> &&callback) {
    nlohmann::json Title; 

    Title["title"] = "Create Item";
    Title["item"] = {
        {"id", ""},
        {"title", ""},
        {"description", ""},
        {"status", ""}
    };
    inja::Environment env;
    // Just load the HTML with an empty table
    auto tpl = env.parse_template("../views/form.html");

    // No data passed yet
    std::string result = env.render(tpl, Title);

    auto resp = HttpResponse::newHttpResponse();
    resp->setContentTypeCode(CT_TEXT_HTML);
    resp->setBody(result);
    callback(resp);
}

void Crud::getAllItems(const HttpRequestPtr &req,
                       std::function<void(const HttpResponsePtr &)> &&callback) {
    sqlite3 *db;
    sqlite3_stmt *stmt;
    nlohmann::json items = nlohmann::json::array();

    // Open the database
    if (sqlite3_open("../items.db", &db) != SQLITE_OK) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k500InternalServerError);
        callback(resp);
        return;
    }

    // Prepare the SQL statement
    const char *sql = "SELECT id, title, description ,status FROM items";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "SQL error: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k500InternalServerError);
        callback(resp);
        return;
    }

    // Execute the query and build the JSON array
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        std::string title(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1)));
        std::string description(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2)));
        std::string status(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3)));

        items.push_back({{"id", id}, {"title", title}, {"description", description}, {"status", status}});
    }

    // Finalize the statement and close the database
    sqlite3_finalize(stmt);
    sqlite3_close(db);

    // Create the HTTP response with the JSON data
    auto resp = HttpResponse::newHttpResponse();
    resp->setContentTypeCode(CT_APPLICATION_JSON);
    resp->setBody(items.dump());

    // Call the callback with the response
    callback(resp);
}

void Crud::deleteItem(const HttpRequestPtr &req,
                      std::function<void(const HttpResponsePtr &)> &&callback,
                      int id) {
    sqlite3 *db;
    sqlite3_stmt *stmt;

    // Open DB in root directory
    if (sqlite3_open("../items.db", &db) != SQLITE_OK) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k500InternalServerError);
        callback(resp);
        return;
    }

    const char *sql = "DELETE FROM items WHERE id = ?";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, id);
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            std::cerr << "Failed to delete item: " << sqlite3_errmsg(db) << std::endl;
        }
        sqlite3_finalize(stmt);
    } else {
        std::cerr << "Failed to prepare delete statement: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k500InternalServerError);
        callback(resp);
        return;
    }

    sqlite3_close(db);

    auto resp = HttpResponse::newHttpResponse();
    resp->setStatusCode(k200OK);
    callback(resp);
}

void Crud::editPage(const HttpRequestPtr &req,
                    std::function<void(const HttpResponsePtr &)> &&callback,
                    int id) {
    sqlite3 *db = openDB();
    if (!db) {
        callback(HttpResponse::newHttpResponse(k500InternalServerError, CT_TEXT_PLAIN));

        return;
    }

    nlohmann::json item = {{"id", ""}, {"title", ""}, {"description", ""}, {"status", ""}};
    const char *sql = "SELECT id, title, description , status FROM items WHERE id = ?";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, id);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            item["id"] = sqlite3_column_int(stmt, 0);
            item["title"] = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
            item["description"] = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
            item["status"] = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
        }
        sqlite3_finalize(stmt);
    }
    sqlite3_close(db);

    inja::Environment env;
    auto tpl = env.parse_template("../views/form.html");
    std::string result = env.render(tpl, {{"title", "Edit Item"}, {"item", item}});

    auto resp = HttpResponse::newHttpResponse();
    resp->setContentTypeCode(CT_TEXT_HTML);
    resp->setBody(result);
    callback(resp);
}

void Crud::createItem(const HttpRequestPtr &req,
                      std::function<void(const HttpResponsePtr &)> &&callback) {
    try {
        auto json = req->getJsonObject();
        if (!json || !json->isMember("title") || !json->isMember("description") || !json->isMember("status")) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k400BadRequest);
            resp->setBody("Invalid JSON: 'name' and 'description' are required");
            callback(resp);
            return;
        }

        std::string name = (*json)["title"].asString();
        std::string description = (*json)["description"].asString();
        std::string status = (*json)["status"].asString();

        sqlite3 *db = openDB();
        if (!db) {
            callback(HttpResponse::newHttpResponse(k500InternalServerError, CT_TEXT_PLAIN));
            return;
        }

        const char *sql = "INSERT INTO items (title, description, status) VALUES (?, ?,?)";
        sqlite3_stmt *stmt;

        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 2, description.c_str(), -1, SQLITE_TRANSIENT);
             sqlite3_bind_text(stmt, 3, status.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }

        sqlite3_close(db);

        Json::Value res;
        res["status"] = "success";
        res["message"] = "Item created";

        callback(HttpResponse::newHttpJsonResponse(res));
    } catch (...) {
        callback(HttpResponse::newHttpResponse(k500InternalServerError, CT_TEXT_PLAIN));
    }
}

void Crud::updateItem(const HttpRequestPtr &req,
                      std::function<void(const HttpResponsePtr &)> &&callback,
                      int id) {
    try {
        auto json = req->getJsonObject();
        if (!json || !json->isMember("title") || !json->isMember("description") || !json->isMember("status")) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k400BadRequest);
            resp->setBody("Invalid JSON: 'name' and 'description' are required");
            callback(resp);
            return;
        }

        std::string title = (*json)["title"].asString();
        std::string description = (*json)["description"].asString();
        std::string status = (*json)["status"].asString();

        sqlite3 *db = openDB();
        if (!db) {
            callback(HttpResponse::newHttpResponse(k500InternalServerError, CT_TEXT_PLAIN));
            return;
        }

        const char *sql = "UPDATE items SET title = ?, description = ? , status = ? WHERE id = ?";
        sqlite3_stmt *stmt;

        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, title.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 2, description.c_str(), -1, SQLITE_TRANSIENT);
             sqlite3_bind_text(stmt, 3, status.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int(stmt, 4, id);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }

        sqlite3_close(db);

        Json::Value res;
        res["status"] = "success";
        res["message"] = "Item updated";

        callback(HttpResponse::newHttpJsonResponse(res));
    } catch (...) {
        callback(HttpResponse::newHttpResponse(k500InternalServerError, CT_TEXT_PLAIN));
    }
}

void Crud::getItemById(const HttpRequestPtr &req,
                       std::function<void(const HttpResponsePtr &)> &&callback) {
    // Get query parameter `id`
    auto idStr = req->getParameter("id");
    if (idStr.empty()) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Missing 'id' query parameter");
        callback(resp);
        return;
    }

    int id = std::stoi(idStr);
    sqlite3 *db = openDB();
    if (!db) {
        callback(HttpResponse::newHttpResponse(k500InternalServerError, CT_TEXT_PLAIN));
        return;
    }

    const char *sql = "SELECT id, name, description FROM items WHERE id = ?";
    sqlite3_stmt *stmt;
    nlohmann::json item;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, id);

        if (sqlite3_step(stmt) == SQLITE_ROW) {
            item["id"] = sqlite3_column_int(stmt, 0);
            item["name"] = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
            item["description"] = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
        } else {
            // Item not found
            sqlite3_finalize(stmt);
            sqlite3_close(db);
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k404NotFound);
            resp->setBody("Item not found");
            callback(resp);
            return;
        }

        sqlite3_finalize(stmt);
    } else {
        std::cerr << "SQL error: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k500InternalServerError);
        callback(resp);
        return;
    }

    sqlite3_close(db);

    auto resp = HttpResponse::newHttpResponse();
    resp->setContentTypeCode(CT_APPLICATION_JSON);
    resp->setBody(item.dump());
    callback(resp);
}

