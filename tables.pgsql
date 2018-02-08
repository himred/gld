CREATE TABLE greylist (
  ip char(16) NOT NULL default '',
  sender char(242) NOT NULL default '',
  recipient char(242) NOT NULL default '',
  first bigint NOT NULL default '0',
  last bigint NOT NULL default '0',
  n bigint NOT NULL default '0',
  PRIMARY KEY  (ip,sender,recipient)
);
    

CREATE TABLE whitelist (
  mail char(242) NOT NULL default '',
  comment char(242) NOT NULL default '',
  PRIMARY KEY  (mail)
);

CREATE OR REPLACE FUNCTION unix_timestamp() RETURNS INTEGER AS '
	SELECT date_part(''epoch'', TIMESTAMP ''now'')::INTEGER;
' LANGUAGE SQL;

CREATE OR REPLACE FUNCTION unix_timestamp(TIMESTAMP) RETURNS INTEGER AS '
	SELECT date_part(''epoch'', $1)::INTEGER;
' LANGUAGE SQL;
