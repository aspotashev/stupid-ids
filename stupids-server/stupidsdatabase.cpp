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

#include <libcouchbase/api3.h>
#include <libcouchbase/views.h>

#include <utility>

class RequestBase
{
public:
    virtual void callbackRow(const lcb_RESPVIEWQUERY *rv) = 0;
    virtual void callbackFinal(const lcb_RESPVIEWQUERY *rv) = 0;
};

class GetIdsRequest
{
public:
    GetIdsRequest();

    virtual void callbackRow(const lcb_RESPVIEWQUERY *rv);
    virtual void callbackFinal(const lcb_RESPVIEWQUERY *rv);

    std::pair<int, int> m_res;
};

GetIdsRequest::GetIdsRequest()
    : m_res(std::make_pair<int, int>(0, -1))
{
}

void GetIdsRequest::callbackRow(const lcb_RESPVIEWQUERY* rv)
{
    if (rv->rc == 0) {
        std::string value(rv->value, rv->nvalue);

        // TBD: use rapidjson
        int first_id = -1;
        int sz = -1;
        assert(sscanf(value.c_str(), "[%d,%d]", &first_id, &sz) == 2);

        m_res = std::pair<int, int>(first_id, sz);
        printf("first_id = %d\n", first_id);
        printf("sz = %d\n", sz);
    } else {
        // failed
    }
}

void GetIdsRequest::callbackFinal(const lcb_RESPVIEWQUERY* rv)
{
}

extern "C" {
static void viewCallback(lcb_t, int, const lcb_RESPVIEWQUERY *rv)
{
    RequestBase* request = reinterpret_cast<RequestBase*>(rv->cookie);

    if (rv->rflags & LCB_RESP_F_FINAL) {
        request->callbackFinal(rv);
    } else {
        request->callbackRow(rv);
    }
}
}

StupidsDatabase::StupidsDatabase()
{
    lcb_create_st cropts;
    memset(&cropts, 0, sizeof cropts);
    const char *connstr = "couchbase://localhost/stupids";

    cropts.version = 3;
    cropts.v.v3.connstr = connstr;
    lcb_error_t rc;
    rc = lcb_create(&m_instance, &cropts);
    assert(rc == LCB_SUCCESS);
    rc = lcb_connect(m_instance);
    assert(rc == LCB_SUCCESS);
    lcb_wait(m_instance);
    assert(lcb_get_bootstrap_status(m_instance) == LCB_SUCCESS);
}

StupidsDatabase::~StupidsDatabase()
{
    lcb_destroy(m_instance);
}

std::pair<int, int> StupidsDatabase::getFirstId(const GitOid& tp_hash)
{
    lcb_CMDVIEWQUERY vq = { 0 };
    std::string dName = "stupids-couchbase";
    std::string vName = "first_id_by_tp_hash";
    std::string options = std::string("reduce=false&key=\"") + tp_hash.toString() + std::string("\"");

    vq.callback = viewCallback;
    vq.ddoc = dName.c_str();
    vq.nddoc = dName.length();
    vq.view = vName.c_str();
    vq.nview = vName.length();
    vq.optstr = options.c_str();
    vq.noptstr = options.size();

    vq.cmdflags = LCB_CMDVIEWQUERY_F_INCLUDE_DOCS;

    GetIdsRequest request;

    lcb_error_t rc = lcb_view_query(m_instance, &request, &vq);
    assert(rc == LCB_SUCCESS);
    lcb_wait(m_instance);

    return request.m_res;
}
