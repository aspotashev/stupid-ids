/*
 * stupid-ids
 * Copyright 2015  Alexander Potashev <aspotashev@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "stupidsdatabase.h"

#include <boost/network/protocol/http/client.hpp>

StupidsDatabase::StupidsDatabase()
{
}

StupidsDatabase::~StupidsDatabase()
{
}

std::pair<int, int> StupidsDatabase::getFirstId(const GitOid& tp_hash)
{
    using namespace boost::network;
    using namespace boost::network::http;

    std::stringstream ss;
    ss << "http://127.0.0.1:1235/tphash?tphash=" << tp_hash.toString();

    client::request request_(ss.str());
    request_ << header("Connection", "close");
    client client_;
    client::response response_ = client_.get(request_);
    std::string body_ = body(response_);

    int id_count = -1;
    int first_id = -1;
    assert(2 == sscanf(body_.c_str(), "{\"id_count\": %d, \"first_id\": %d}", &id_count, &first_id));

    return std::make_pair(first_id, id_count);
}
