"""
Copyright 2018 Accelize
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
   http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
"""
import json
import requests

from requests.adapters import HTTPAdapter
from requests.packages.urllib3.util.retry import Retry


class WSListFunction(object):

    def __init__(self, url=None, login=None, password=None, token=None):
        self.url = url
        self.login = login
        self.password = password
        self.token = token

    def _get_user_token_raw(self):
        r = requests.post(self.url + '/o/token/?grant_type=client_credentials',
                          auth=(self.login, self.password),
                          headers={'Content-Type': 'application/json'})
        json_acceptable_string = r.content.decode("latin1").replace("'", "\"")
        try:
            text = json.loads(json_acceptable_string)
        except:
            text = json_acceptable_string

        return text, r.status_code

    def _get_user_token(self):
        text, status_code = self._get_user_token_raw()
        assert status_code == 200
        assert 'access_token' in text
        self.token = text['access_token']

    def _authentifed_call(self, method, url, data=None, headers={}):
        headers['Authorization'] = "Bearer " + str(self.token)
        headers['Content-Type'] = "application/json"
        s = requests.Session()
        retries = Retry(total=10, backoff_factor=1, status_forcelist=[ 502, 503, 504 ])
        s.mount('http://', HTTPAdapter(max_retries=retries))
        s.mount('https://', HTTPAdapter(max_retries=retries))
        r = s.request(method, self.url + url, data=json.dumps(data), headers=headers)
        try:
            text = json.loads(r.content)
        except:
            text = r.content
        return text, r.status_code

    def _download_authentifed_call(self, method, url, data, headers={}):
        headers['Authorization'] = "Bearer " + str(self.token)
        r = requests.request(method, self.url + url, data=data, headers=headers, stream=True)
        return r.content, r.status_code

    def get_authentification_token(self):
        return self.token

    def application_create(self, data):
        #    url(r'^auth/createapplication/', APIMetering.create_application),
        response, status = self._authentifed_call("POST", "/auth/createapplication/", data=data)
        return response, status

    def application_list(self, data):
        #        url(r'^auth/listapplication/', APIMetering.list_application),
        response, status = self._authentifed_call("POST", "/auth/listapplication/", data=data)
        return response, status

    def application_delete(self, data):
        #            url(r'^auth/deleteapplication/', APIMetering.delete_application),
        response, status = self._authentifed_call("POST", "/auth/deleteapplication/", data=data)
        return response, status

    def metering_information(self, data):
        #            url(r'^auth/meteringinformation/', APIMetering.user_metering),
        response, status = self._authentifed_call("POST", "/auth/meteringinformation/", data=data)
        return response, status

    def floatingallinformation(self, data):
        #            url(r'^auth/meteringinformation/', APIMetering.user_metering),
        response, status = self._authentifed_call("POST", "/auth/floatingallinformation/", data=data)
        return response, status

    def vendorallinformation(self, data):
        #            url(r'^auth/vendorallinformation/', APIMetering.user_metering),
        response, status = self._authentifed_call("POST", "/auth/vendorallinformation/", data=data)
        return response, status

    def vendorcheckinformation(self, data):
        #            url(r'^auth/vendorcheckinformation/', APIMetering.user_metering),
        response, status = self._authentifed_call("POST", "/auth/vendorcheckinformation/", data=data)
        return response, status

    def nodelock_information(self, data):
        #            url(r'^auth/meteringinformation/', APIMetering.nodelockassociated),
        response, status = self._authentifed_call("POST", "/auth/nodelockassociated/", data=data)
        return response, status

    def metering_get_license_timeout(self):
        response, status = self._authentifed_call("GET", "/auth/getlicensetimeout/")
        return response, status

    def metering_lastinformation(self, data):
        #                url(r'^auth/lastmeteringinformation/', APIMetering.last_metering),
        response, status = self._authentifed_call("POST", "/auth/lastmeteringinformation/", data=data)
        return response, status

    def metering_getlicense(self, data):
        #                  url(r'^auth/metering/genlicense/', APIMetering.get_license ),
        response, status = self._authentifed_call("POST", "/auth/metering/genlicense/", data=data)
        return response, status

    def metering_getlicense_random(self, data):
        #                  url(r'^auth/metering/genlicense/', APIMetering.get_license ),
        response, status = self._authentifed_call("POST", "/auth/tests/genlicense/", data=data)
        return response, status

    def nodelock_getlicense(self, data):
        response, status = self._authentifed_call("POST", "/auth/metering/genlicense/", data=data)
        return response, status

    def configuration_list(self):
        #            url(r'^auth/getlastcspconfiguration/', APIMetering.get_last_configuration),
        response, status = self._authentifed_call("GET", "/auth/getlastcspconfiguration/")
        return response, status

    def configuration_create(self, data):
        #           url(r'^auth/createcspconfiguration/', APIMetering.configuration),
        response, status = self._authentifed_call("POST", "/auth/cspconfiguration/", data=data)
        return response, status

    def configuration_delete(self, data):
        #           url(r'^auth/createcspconfiguration/', APIMetering.configuration),
        response, status = self._authentifed_call("DELETE", "/auth/cspconfiguration/", data=data)
        return response, status

    def user_update_list(self):
        #           url(r'^auth/updateuserlist/', APIMetering.update_user_list_from_accelstore),
        response, status = self._authentifed_call("GET", "/auth/updateuserlist/")
        return response, status

    def remove_test_session(self):
        #           url(r'^auth/updateuserlist/', APIMetering.update_user_list_from_accelstore),
        response, status = self._authentifed_call("GET", "/auth/metering/rmsession/")
        return response, status

    def clear_token(self):
        #             url(r'^auth/admin/clear_token/', APIMetering.clear_token),
        response, status = self._authentifed_call("GET", "/auth/admin/clear_token/")
        return response, status

    def user_single_user(self, data):
        #       url(r'^auth/userupdate/', APIMetering.user_update),
        response, status = self._authentifed_call("POST", "/auth/userupdate/", data=data)
        return response, status

    def user_single_user_card(self, data):
        #          url(r'^auth/usercardupdate/', APIMetering.user_card_update),
        response, status = self._authentifed_call("POST", "/auth/usercardupdate/", data=data)
        return response, status

    def ip_create(self, data):
        #            url(r'^auth/ip/create/', APIMetering.CreateIP),
        response, status = self._authentifed_call("POST", "/auth/ip/create/", data=data)
        return response, status

    def ip_delete(self, data):
        #            url(r'^auth/ip/create/', APIMetering.CreateIP),
        response, status = self._authentifed_call("POST", "/auth/ip/delete/", data=data)
        return response, status

    def ip_get_hdk(self, data):
        # url(r'^auth/ip/hdk/', APIMetering.get_HDK),
        response, status = self._download_authentifed_call("POST", "/auth/ip/hdk/", data=data)
        return response, status

    def ip_create_get_hdk(self, data):
        # url(r'^auth/ip/hdk/', APIMetering.get_HDK),
        response, status = self._download_authentifed_call("POST", "/auth/ip/get_create_hdk/", data=data)
        return response, status

    def server_get_version(self):
        # url(r'^version/', APIMetering.get_version),
        response, status = self._authentifed_call("GET", "/version/")
        return response, status

    def metering_synips(self):
        # url(r'^auth/metering/syncips/', APIMetering.sync_IP_with_LGDN),
        response, status = self._authentifed_call("GET", "/auth/metering/syncips/")
        return response, status

    def remove_product_information(self, data):
        # url(r'^auth/metering/rmthissession/', APIMetering.remove_product_information),
        """
        print('[remove_product_information] data=', data)
        print('[remove_product_information] url=', self.url)
        print('[remove_product_information] login=', self.login)
        print('[remove_product_information] password=', self.password)
        print('[remove_product_information] self=', self.__dict__)
        """
        response, status = self._authentifed_call("POST", "/auth/metering/archiveduserproductinfo/", data=data)
        return response, status

    def get_user_token(self, email):
        # url(r'^auth/admin/get_token/', APIMetering.remove_product_information),
        response, status = self._authentifed_call("POST", "/auth/admin/get_token/", data={"email":email})
        return response, status

    def object_manager(self, api_object, method, data, urlsuffix=''):
        if urlsuffix == '' or urlsuffix.endswith('/'):
            urlsuffix = urlsuffix+"?from=drmportalpreview.accelize.com"
        else:
            urlsuffix = urlsuffix+"&from=drmportalpreview.accelize.com"
        response, status = self._authentifed_call(method, "/auth/objects/%s/%s" % (api_object, urlsuffix), data=data)
        return response, status
