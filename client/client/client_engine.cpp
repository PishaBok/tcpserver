#include <client/client_engine.hpp>
#include "client_engine.hpp"


ClientEngine::ClientEngine(QObject *parent)
    : QObject{parent}, _currentPage{1},
    _buttonToFunc
    {
        {Button::stepBack, std::bind(&ClientEngine::stepBack, this)},
        {Button::stepForward, std::bind(&ClientEngine::stepForward, this)},
        {Button::filter, std::bind(&ClientEngine::filterButton, this)},
        {Button::search, std::bind(&ClientEngine::searchButton, this)}
    },
    _responseToFunc
    {
        {RequestType::page, std::bind(&ClientEngine::pageResponse, this, std::placeholders::_1)}
    }
{}

ClientEngine::~ClientEngine()
{}

void ClientEngine::initSocket()
{
    _socket = std::make_unique<Socket>("localhost", 9999);

    connect(this, &ClientEngine::sendToServer, _socket.get(), &Socket::sendToServer);
    connect(_socket.get(), &Socket::processResponse, this, &ClientEngine::processResponse);

    pageButton();
}

void ClientEngine::processButton(const Button button)
{
    _buttonToFunc[button]();
}

void ClientEngine::pageButton()
{
    // Поиск в кэше
    if (findInCache(_currentPage)) {return;}
    // Создание запроса
    PageRequest request(_currentPage, _filterMap);
    // Отправка запроса на сервер
    emit sendToServer(QJsonDocument(request.serialize()).toJson());
}

void ClientEngine::filterButton()
{

}

void ClientEngine::searchButton()
{

}


void ClientEngine::processResponse(const QJsonObject& json)
{
    // Получаем тип ответа сервера
    RequestType type = static_cast<RequestType>(json.value("type").toInt());

    auto response = _responseFactory[type]();
    response->deserialize(json);

    _responseToFunc[type](std::move(response));
}

void ClientEngine::pageResponse(const std::unique_ptr<Response>& response)
{
    _savedPages.clear();

    QJsonObject data = response->data();

    QJsonArray modelsArr = data.value("models").toArray();
    for (const QJsonValue& value: modelsArr)
    {
        QJsonObject jsonObj = value.toObject();
        clib::TableModel model;
        model.loadJson(jsonObj.value("model").toObject());
        _savedPages[jsonObj.value("page").toInt()] = model;
    }

    emit updatePage(_savedPages.at(_currentPage));
}





void ClientEngine::stepBack()
{
    if (_currentPage > 1)
    {
        --_currentPage;
        pageButton();
    }
}

void ClientEngine::stepForward()
{
    if (_currentPage < _pageCount)
    {
        ++_currentPage;
        pageButton();
    }
}

bool ClientEngine::findInCache(const int pageN)
{
    auto found = _savedPages.find(pageN);
    if (found != _savedPages.end())
    {
        emit updatePage(found->second);
        return true;
    }

    return false;
}
