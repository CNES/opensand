# Installation via APT

  As with all other OpenSAND packages, the web configuration tool is available in the
[Net4Sat PPA on GitHub](https://github.com/CNES/net4sat-packages). To install packets
from there you must:

 * Trust the Net4Sat GPG signing key:
   * `curl -sS https://raw.githubusercontent.com/CNES/net4sat-packages/master/gpg/net4sat.gpg.key | sudo apt-key add -`
 * Add the repository to your APT sources:
   * `echo "deb https://raw.githubusercontent.com/CNES/net4sat-packages/master/ focal stable" | sudo tee /etc/apt/sources.list.d/github.net4sat.list`
 * Install the package:
   * `sudo apt-get update && sudo apt-get install opensand-deploy`
 * Browse to `localhost:80` to access the web configuration tool.

# Installation from sources

 * Install NodeJS and Yarn to compile the frontend:
   * `curl -fsSL https://deb.nodesource.com/setup_14.x | sudo -E bash -`
   * `curl -sL https://dl.yarnpkg.com/debian/pubkey.gpg | gpg --dearmor | sudo tee /usr/share/keyrings/yarnkey.gpg >/dev/null`
   * `echo "deb [signed-by=/usr/share/keyrings/yarnkey.gpg] https://dl.yarnpkg.com/debian stable main" | sudo tee /etc/apt/sources.list.d/yarn.list`
   * `sudo apt-get update && sudo apt-get install nodejs yarn`
 * Build the frontend:
   * `cd opensand/opensand-deploy/src/frontend; yarn install && yarn build`
 * Install OpenSAND to generate configuration files:
   * `sudo apt-get install opensand`
   * `opensand -g opensand/opensand-deploy/src/backend/`
 * Install uWSGI to serve both the frontend and the backend:
   * `sudo apt-get install uwsgi`
   * `uwsgi --plugin python3 --http-socket :8080 --wsgi-file opensand/opensand-deploy/src/backend/backend.py --callable app --check-static opensand/opensand-deploy/src/frontend/build/ --master --workers 5 --die-on-term --wsgi-disable-file-wrapper`
 * Browse to `localhost:8080` to access the web configuration tool.
