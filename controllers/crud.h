#pragma once
#include <drogon/HttpController.h>
using namespace drogon;

class Crud : public drogon::HttpController<Crud> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(Crud::index, "/items", Get);
    ADD_METHOD_TO(Crud::getAllItems, "/api/items", Get);
    ADD_METHOD_TO(Crud::getItemById, "/items/get", Get); 
    ADD_METHOD_TO(Crud::deleteItem, "/api/items/{id}", Delete);
    ADD_METHOD_TO(Crud::editPage, "/items/{id}", Get);
    ADD_METHOD_TO(Crud::createPage, "/items/create", Get);

    ADD_METHOD_TO(Crud::createItem, "/api/items", Post);   // POST for creating
    ADD_METHOD_TO(Crud::updateItem, "/api/items/{id}", Put); // PUT for updating
   

    
    METHOD_LIST_END

    void index(const HttpRequestPtr &req,
               std::function<void(const HttpResponsePtr &)> &&callback);

    void getAllItems(const HttpRequestPtr &req,
                     std::function<void(const HttpResponsePtr &)> &&callback);

    void deleteItem(const HttpRequestPtr &req,
                std::function<void(const HttpResponsePtr &)> &&callback,
                int id);

    void createPage(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);

    void updateItem(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, int id);

    void editPage(const HttpRequestPtr &req,
              std::function<void(const HttpResponsePtr &)> &&callback,
              int id);

    void createItem(const HttpRequestPtr &req,
                    std::function<void(const HttpResponsePtr &)> &&callback);  
       void getItemById(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);  

};



