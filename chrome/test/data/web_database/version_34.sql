PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE meta(key LONGVARCHAR NOT NULL UNIQUE PRIMARY KEY,value LONGVARCHAR);
INSERT INTO "meta" VALUES('Default Search Provider ID','2');
INSERT INTO "meta" VALUES('Builtin Keyword Version','33');
INSERT INTO "meta" VALUES('version','34');
INSERT INTO "meta" VALUES('last_compatible_version','34');
CREATE TABLE keywords (id INTEGER PRIMARY KEY,short_name VARCHAR NOT NULL,keyword VARCHAR NOT NULL,favicon_url VARCHAR NOT NULL,url VARCHAR NOT NULL,show_in_default_list INTEGER,safe_for_autoreplace INTEGER,originating_url VARCHAR,date_created INTEGER DEFAULT 0,usage_count INTEGER DEFAULT 0,input_encodings VARCHAR,suggest_url VARCHAR,prepopulate_id INTEGER DEFAULT 0,autogenerate_keyword INTEGER DEFAULT 0,logo_id INTEGER DEFAULT 0,created_by_policy INTEGER DEFAULT 0,instant_url VARCHAR);
INSERT INTO "keywords" VALUES(2,'Google','google.com','http://www.google.com/favicon.ico','{google:baseURL}search?{google:RLZ}{google:acceptedSuggestion}{google:originalQueryForSuggestion}sourceid=chrome&ie={inputEncoding}&q={searchTerms}',1,1,'',0,0,'UTF-8','{google:baseSuggestURL}search?client=chrome&hl={language}&q={searchTerms}',1,1,6256,0,'{google:baseURL}webhp?{google:RLZ}sourceid=chrome-instant&ie={inputEncoding}&ion=1{searchTerms}&nord=1');
INSERT INTO "keywords" VALUES(3,'Yahoo!','yahoo.com','http://search.yahoo.com/favicon.ico','http://search.yahoo.com/search?ei={inputEncoding}&fr=crmas&p={searchTerms}',1,1,'',0,0,'UTF-8','http://ff.search.yahoo.com/gossip?output=fxjson&command={searchTerms}',2,0,6273,0,'');
INSERT INTO "keywords" VALUES(4,'Bing','bing.com','http://www.bing.com/s/wlflag.ico','http://www.bing.com/search?setmkt=en-US&q={searchTerms}',1,1,'',0,0,'UTF-8','http://api.bing.com/osjson.aspx?query={searchTerms}&language={language}',3,0,6250,0,'');
INSERT INTO "keywords" VALUES(5,'Search the web (Babylon)','search.babylon.com','','http://search.babylon.com/web/{searchTerms}?babsrc=browsersearch',1,0,'',1299093361,0,'','',0,0,0,0,'');
CREATE TABLE logins (origin_url VARCHAR NOT NULL, action_url VARCHAR, username_element VARCHAR, username_value VARCHAR, password_element VARCHAR, password_value BLOB, submit_element VARCHAR, signon_realm VARCHAR NOT NULL,ssl_valid INTEGER NOT NULL,preferred INTEGER NOT NULL,date_created INTEGER NOT NULL,blacklisted_by_user INTEGER NOT NULL,scheme INTEGER NOT NULL,UNIQUE (origin_url, username_element, username_value, password_element, submit_element, signon_realm));
CREATE TABLE ie7_logins (url_hash VARCHAR NOT NULL, password_value BLOB, date_created INTEGER NOT NULL,UNIQUE (url_hash));
CREATE TABLE web_app_icons (url LONGVARCHAR,width int,height int,image BLOB, UNIQUE (url, width, height));
CREATE TABLE web_apps (url LONGVARCHAR UNIQUE,has_all_images INTEGER NOT NULL);
CREATE TABLE autofill (name VARCHAR, value VARCHAR, value_lower VARCHAR, pair_id INTEGER PRIMARY KEY, count INTEGER DEFAULT 1);
INSERT INTO "autofill" VALUES('firstname','David','david',1,1);
INSERT INTO "autofill" VALUES('lastname','Holloway','holloway',2,1);
INSERT INTO "autofill" VALUES('email','d@gmail.com','d@gmail.com',3,1);
INSERT INTO "autofill" VALUES('phone','415-551-2222','415-551-2222',4,1);
INSERT INTO "autofill" VALUES('fax','415-551-2222','415-551-2222',5,1);
INSERT INTO "autofill" VALUES('address','1122 Boogie Boogie Avenue','1122 boogie boogie avenue',6,1);
INSERT INTO "autofill" VALUES('city','San Francisco','san francisco',7,1);
INSERT INTO "autofill" VALUES('zipcode','11001','11001',8,1);
INSERT INTO "autofill" VALUES('country','UK','uk',9,1);
CREATE TABLE autofill_dates ( pair_id INTEGER DEFAULT 0, date_created INTEGER DEFAULT 0);
INSERT INTO "autofill_dates" VALUES(1,1299093389);
INSERT INTO "autofill_dates" VALUES(2,1299093389);
INSERT INTO "autofill_dates" VALUES(3,1299093389);
INSERT INTO "autofill_dates" VALUES(4,1299093389);
INSERT INTO "autofill_dates" VALUES(5,1299093389);
INSERT INTO "autofill_dates" VALUES(6,1299093389);
INSERT INTO "autofill_dates" VALUES(7,1299093389);
INSERT INTO "autofill_dates" VALUES(8,1299093389);
INSERT INTO "autofill_dates" VALUES(9,1299093389);
CREATE TABLE autofill_profiles ( guid VARCHAR PRIMARY KEY, company_name VARCHAR, address_line_1 VARCHAR, address_line_2 VARCHAR, city VARCHAR, state VARCHAR, zipcode VARCHAR, country VARCHAR, date_modified INTEGER NOT NULL DEFAULT 0, country_code VARCHAR);
INSERT INTO "autofill_profiles" VALUES('F19484ED-363F-4506-997E-E0F23EA834AB','','1122 Boogie Boogie Avenue','','San Francisco','?','11001','UK',1299093389,'UK');
CREATE TABLE autofill_profile_names ( guid VARCHAR, first_name VARCHAR, middle_name VARCHAR, last_name VARCHAR);
INSERT INTO "autofill_profile_names" VALUES('F19484ED-363F-4506-997E-E0F23EA834AB','David','','Holloway');
CREATE TABLE autofill_profile_emails ( guid VARCHAR, email VARCHAR);
INSERT INTO "autofill_profile_emails" VALUES('F19484ED-363F-4506-997E-E0F23EA834AB','d@gmail.com');
CREATE TABLE autofill_profile_phones ( guid VARCHAR, type INTEGER DEFAULT 0, number VARCHAR);
INSERT INTO "autofill_profile_phones" VALUES('F19484ED-363F-4506-997E-E0F23EA834AB',0,'4155512222');
INSERT INTO "autofill_profile_phones" VALUES('F19484ED-363F-4506-997E-E0F23EA834AB',1,'4155512222');
CREATE TABLE credit_cards ( guid VARCHAR PRIMARY KEY, name_on_card VARCHAR, expiration_month INTEGER, expiration_year INTEGER, card_number_encrypted BLOB, date_modified INTEGER NOT NULL DEFAULT 0);
CREATE TABLE token_service (service VARCHAR PRIMARY KEY NOT NULL,encrypted_token BLOB);
CREATE INDEX logins_signon ON logins (signon_realm);
CREATE INDEX ie7_logins_hash ON ie7_logins (url_hash);
CREATE INDEX web_apps_url_index ON web_apps (url);
CREATE INDEX autofill_name ON autofill (name);
CREATE INDEX autofill_name_value_lower ON autofill (name, value_lower);
CREATE INDEX autofill_dates_pair_id ON autofill_dates (pair_id);
COMMIT;
