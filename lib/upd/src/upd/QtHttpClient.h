/*
 Copyright (C) 2025 Kristian Duske

 This file is part of TrenchBroom.

 TrenchBroom is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 TrenchBroom is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with TrenchBroom. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "upd/HttpClient.h"

class QNetworkAccessManager;

namespace upd
{

class QtHttpClient : public HttpClient
{
private:
  QNetworkAccessManager& m_networkManager;

public:
  explicit QtHttpClient(QNetworkAccessManager& networkManager);

  HttpOperation* get(
    const QUrl& url, GetCallback getCallback, ErrorCallback errorCallback) const override;

  HttpOperation* download(
    const QUrl& url,
    DownloadCallback downloadCallback,
    ErrorCallback errorCallback) const override;
};

} // namespace upd
