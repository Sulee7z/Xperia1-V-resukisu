import sys, configuration_pb2, iddd_pb2, agent_pb2, os

def generate_file(filename, configs, cert = None):
    cp = configuration_pb2.ConfigPackage()
    for c in configs:
        part = cp.parts.add()
        part.package = c[0]
        part.name = c[1]
        part.payload = c[2]

    import gzip
    from cStringIO import StringIO
    buf = StringIO()
    gz = gzip.GzipFile(mode = "wb", fileobj = buf, compresslevel = 9)
    gz.write(cp.SerializeToString())
    gz.close()

    # Patch the gzip stream and remove the timestamp.
    gzdata = buf.getvalue()
    patched_gz = gzdata[0:4] + "\0\0\0\0" + gzdata[8:]

    cr = configuration_pb2.ConfigResponse()
    cr.compressed_data = patched_gz

    if cert != None:
        # Sign the compressed data using openssl
        d = file("_tmp_in", "wb")
        d.write(patched_gz)
        d.close()
        os.system("openssl dgst -sha1 -sign ../test-certs/%s -out _tmp_out _tmp_in" % cert)
        sig = file("_tmp_out", "rb").read()
        cr.signature = sig
        #os.system("sha1sum _tmp_in > %s_sha1.txt" % filename)
        #os.system("sha1sum _tmp_out >> %s_sha1.txt" % filename)
        os.unlink("_tmp_in")
        os.unlink("_tmp_out")

    f = file(filename, "wb")
    binary = cr.SerializeToString()
    f.write(binary)
    sys.stderr.write("%s: %d bytes.\n" % (filename, len(binary)))

# TC-0007 - unsigned_conf (also for TC-0010, TC-0002 and TC-0003)

bigfile = "abcdefghij" * 1000

exclude_filter = idd.iddd_pb2.FilterConfig()
exclude_filter.event_filter.type = idd.iddd_pb2.Filter.EXCLUDE
exclude_filter.counter_filter.type = idd.iddd_pb2.Filter.EXCLUDE
exclude_filter.staticdata_filter.type = idd.iddd_pb2.Filter.EXCLUDE

configs = (
    ("foo", "bar", "gazonk-contents"),
    ("idd", "logreader", ""),
    ("iddd", "testfile", "abrakadabra"),
    ("other", "big.file", bigfile),
    ("iddd", "FilterConfig", exclude_filter.SerializeToString())
    )

generate_file("tc-0007-data/unsigned_conf", configs)
generate_file("tc-0010-data/unsigned_conf", configs)
generate_file("tc-0002-data/unsigned_conf", configs)
generate_file("tc-0003-data/unsigned_conf", configs)

# TC-0010 - signed_conf & bad_signed_conf

generate_file("tc-0010-data/signed_conf", configs, "../test-certs/config-priv.pem")
generate_file("tc-0010-data/badsigned_conf", configs, "../test-certs/report-priv.pem")

# TC-0007 - unsigned_conf2 (also for TC-0010)

configs = (
    ("idd", "logreader", ""),
    ("iddd", "testfile", "abrakadabra"),
    ("other", "big.file", "smaller file"),
    ("new", "file", "with new contents"),
    ("iddd", "FilterConfig", exclude_filter.SerializeToString())
    )

generate_file("tc-0007-data/unsigned_conf2", configs)
generate_file("tc-0010-data/unsigned_conf2", configs)

# TC-0010 - signed_conf2 & bad_signed_conf2

generate_file("tc-0010-data/signed_conf2", configs, "../test-certs/config-priv.pem")
generate_file("tc-0010-data/badsigned_conf2", configs, "../test-certs/report-priv.pem")

# TC-0007 - maxsizes_conf

iddd_settings = idd.iddd_pb2.Settings()
iddd_settings.max_report_size = 1000
iddd_settings.max_output_dir_size = 2000

configs = [("iddd", "Settings", iddd_settings.SerializeToString()),
           ("iddd", "FilterConfig", exclude_filter.SerializeToString())]

generate_file("tc-0007-data/maxsizes_conf", configs)

# TC-0009 - no-config

configs = []

generate_file("tc-0009-data/no_config", configs)


# TC-0009 - empty_exclude_config

configs = [("iddd", "FilterConfig", exclude_filter.SerializeToString())]

generate_file("tc-0009-data/empty_exclude_config", configs)

# TC-0009 - simple_exclude_config

filter = idd.iddd_pb2.FilterConfig()
filter.event_filter.type = idd.iddd_pb2.Filter.EXCLUDE
filter.counter_filter.type = idd.iddd_pb2.Filter.EXCLUDE
filter.staticdata_filter.type = idd.iddd_pb2.Filter.EXCLUDE

item = filter.event_filter.item.add()
item.package = "foo"
item.name = "bar"

item = filter.event_filter.item.add()
item.package = "fie"
item.name = "fum2"

item = filter.counter_filter.item.add()
item.package = "cnt.pkg"
item.name = "exclude_name"

item = filter.staticdata_filter.item.add()
item.package = "staticdata.pkg"
item.name = "exclude_name"

configs = [("iddd", "FilterConfig", filter.SerializeToString())]

generate_file("tc-0009-data/simple_exclude_config", configs)

# TC-0009 - wildcard_include_config

filter = idd.iddd_pb2.FilterConfig()
filter.event_filter.type = idd.iddd_pb2.Filter.INCLUDE
filter.counter_filter.type = idd.iddd_pb2.Filter.INCLUDE
filter.staticdata_filter.type = idd.iddd_pb2.Filter.INCLUDE

configs = [("iddd", "FilterConfig", filter.SerializeToString())]
generate_file("tc-0004-data/empty_include.config", configs)

item = filter.event_filter.item.add()
item.package = "foo"
item.name = "bar"

item = filter.event_filter.item.add()
item.package = "fie"
item.name = "*"

item = filter.event_filter.item.add()
item.package = "abrakadabra simsalabim"
item.name = "badabing"

filter.counter_filter.type = idd.iddd_pb2.Filter.INCLUDE

item = filter.counter_filter.item.add()
item.package = "cnt.wildcard"
item.name = "*"

item = filter.counter_filter.item.add()
item.package = "cnt.pkg"
item.name = "include_name"

item = filter.staticdata_filter.item.add()
item.package = "staticdata.wildcard"
item.name = "*"

item = filter.staticdata_filter.item.add()
item.package = "staticdata.pkg"
item.name = "include_name"

configs = [("iddd", "FilterConfig", filter.SerializeToString())]

generate_file("tc-0009-data/wildcard_include_config", configs)

# TC-0010 - idd_config.pem.goodsig/badsig
generate_file("tc-0010-data/good_sig.data", configs)
print("tc-0010-data/good_sig.data.sig")
os.system("openssl dgst -sha1 -sign ../test-certs/config-priv.pem -out tc-0010-data/good_sig.data.sig tc-0010-data/good_sig.data")

generate_file("tc-0010-data/bad_sig.data", configs)
print("tc-0010-data/bad_sig.data.sig")
os.system("openssl dgst -sha1 -sign ../test-certs/report-priv.pem -out tc-0010-data/bad_sig.data.sig tc-0010-data/bad_sig.data")

# TC-0012 - log reader not running

ra = idd.iddd_pb2.RunActions()
ra.run_log_reader = False

configs = [("iddd", "RunActions", ra.SerializeToString())]
generate_file("tc-0012-data/not_active", configs)


# TC-0012 - log reader not running

xd_ra = idd.iddd_pb2.RunActions()
xd_ra.run_log_reader = True

# TC-0012 - log reader - simple rules.
rules = """
+ main warning IddLogTest
"""

configs = [("iddd", "RunActions", xd_ra.SerializeToString()),
           ("idd", "LogReader", rules)]
generate_file("tc-0012-data/simple_rules", configs)

# TC-0012 - log reader - file2probe parse error.

cf = logreader_configuration_pb2.ConfigFilereader()
fp = configFilereader_addprobe(cf, "/data/idd_test/tc-0012-data/file2probe_parse_error_filed_not_found.input", "file2probe_parse_error_filed_not_found", "*", False)
file_2_probe_addfield(fp, cf.IGN)
file_2_probe_addfield(fp, cf.I64, "field_two")
fp = configFilereader_addprobe(cf, "/data/idd_test/tc-0012-data/file2probe_parse_error_int64.input", "file2probe_parse_error_int64", " ", False)
file_2_probe_addfield(fp, cf.I64, "current_pid")
file_2_probe_addfield(fp, cf.I64, "current_comm")
file_2_probe_addfield(fp, cf.I64, "selected_pid")
file_2_probe_addfield(fp, cf.STR, "selected_comm")
file_2_probe_addfield(fp, cf.I64, "selected_oom_score_adj")
fp = configFilereader_addprobe(cf, "/data/idd_test/tc-0012-data/file2probe_parse_error_uint64.input", "file2probe_parse_error_uint64", " ", False)
file_2_probe_addfield(fp, cf.I64, "current_pid")
file_2_probe_addfield(fp, cf.STR, "current_comm")
file_2_probe_addfield(fp, cf.I64, "selected_pid")
file_2_probe_addfield(fp, cf.U64, "selected_comm")
file_2_probe_addfield(fp, cf.I64, "selected_oom_score_adj")
fp = configFilereader_addprobe(cf, "/data/idd_test/tc-0012-data/file2probe_parse_error_unparse.input", "file2probe_parse_error_unparse", " ", False)
file_2_probe_addfield(fp, cf.I64, "current_pid")
file_2_probe_addfield(fp, cf.STR, "current_comm")
file_2_probe_addfield(fp, cf.I64, "selected_pid")
file_2_probe_addfield(fp, cf.STR, "selected_comm")
file_2_probe_addfield(fp, cf.I64, "selected_oom_score_adj")

filter = idd.iddd_pb2.FilterConfig()

filter.event_filter.type = idd.iddd_pb2.Filter.INCLUDE
addfilter(filter.event_filter, "idd.system_logging", "FileMessage")
filter.counter_filter.type = idd.iddd_pb2.Filter.INCLUDE
filter.staticdata_filter.type = idd.iddd_pb2.Filter.INCLUDE

configs = [("iddd", "FilterConfig", filter.SerializeToString()),
           ("logreader", "ConfigFilereader", cf.SerializeToString())]
generate_file("tc-0012-data/file2probe_parse_error.config", configs)

# TC-0012 - log reader - eventlog.

filter = idd.iddd_pb2.FilterConfig()
filter.event_filter.type = idd.iddd_pb2.Filter.INCLUDE
addfilter(filter.event_filter, "idd.system_logging", "LogMessage")
filter.counter_filter.type = idd.iddd_pb2.Filter.INCLUDE
filter.staticdata_filter.type = idd.iddd_pb2.Filter.INCLUDE
cflr = logreader_configuration_pb2.ConfigLogreader()
cflr.exit_on_non_blocking = False
cflr.testmode = True
configLogreader_addAndroidLog(cflr, idd.idd_system_logging_pb2.EVENTS, "/data/idd_test/tc-0012-data/event.input")
rules = """
- events * *
+ events * am_create_service
"""

configs = [("iddd", "FilterConfig", filter.SerializeToString()),
           ("iddd", "RunActions", xd_ra.SerializeToString()),
           ("idd", "LogReader", rules),
           ("idd.system_logging", "ConfigLogreader", cflr.SerializeToString())]
generate_file("tc-0012-data/event.config", configs)

cflr.exit_on_non_blocking = True
rules = """
- events * *
+ events * kpi
"""

configs = [("iddd", "FilterConfig", filter.SerializeToString()),
           ("iddd", "RunActions", xd_ra.SerializeToString()),
           ("idd", "LogReader", rules),
           ("idd.system_logging", "ConfigLogreader", cflr.SerializeToString())]
generate_file("tc-0012-data/event2.config", configs)

# TC-0012 - log reader - LogProbe.

xd_cflr = logreader_configuration_pb2.ConfigLogreader()
xd_cflr.exit_on_non_blocking = False
xd_cflr.testmode = True
configLogreader_addAndroidLog(xd_cflr, idd.idd_system_logging_pb2.MAIN, "/data/idd_test/tc-0012-data/main.input")
configLogreader_addAndroidLog(xd_cflr, idd.idd_system_logging_pb2.EVENTS, "/data/idd_test/tc-0012-data/event.input")
XDrules = """
- main * *
+ main * iddd-compactor free
- events * *
+ events * boot_progress_start
"""
configLogreader_addLogProbe(xd_cflr, idd.iddd_pb2.XD, XDrules)
xd_rules = """
- main * *
+ main * iddd-compactor
- events * *
+ events * am_kill
"""

configs = [("iddd", "FilterConfig", filter.SerializeToString()),
           ("iddd", "RunActions", xd_ra.SerializeToString()),
           ("idd", "LogReader", xd_rules),
           ("idd.system_logging", "ConfigLogreader", xd_cflr.SerializeToString())]
generate_file("tc-0012-data/logprobe.config", configs)

configLogreader_addLogProbe(xd_cflr, idd.iddd_pb2.RCA_COMPACT, XDrules)

configs = [("iddd", "FilterConfig", filter.SerializeToString()),
           ("iddd", "RunActions", xd_ra.SerializeToString()),
           ("idd.system_logging", "ConfigLogreader", xd_cflr.SerializeToString())]
generate_file("tc-0012-data/logprobe2.config", configs)

# TC-9999 - agent configs

s = agent_pb2.AgentConfig()
s.upload_period = 12345
s.config_period = 23456
s.upload_limit = 200*1024
server = s.report_servers.add()
server.url = "http://www.example.com/idd-report/"
server = s.report_servers.add()
server.url = "http://www.example.net/idd-report/"

server = s.config_servers.add()
server.url = "http://www.example.net/idd-config/"

server = s.download_servers.add()
server.url = "http://www.example.com/idd-download/%s"

server = s.download_servers.add()
server.url = "http://www.example.net/idd-download/%s"

configs = [("idd", "AgentConfig", s.SerializeToString())]

generate_file("tc-9999/agent_config_1", configs, "../test-certs/config-priv.pem")

s = agent_pb2.AgentConfig()
s.upload_period = 0
s.config_period = 30*86400
s.upload_limit = 0

server = s.config_servers.add()
server.url = "http://no-soup-for-you.com"

server = s.config_servers.add()
server.url = "http://www.example.com/idd-hibernating-config/"

server = s.download_servers.add()
server.url = "http://www.example.com/idd-hibernating-download/%s"

configs = [("idd", "AgentConfig", s.SerializeToString()),
           ("some", "other", "config")]

generate_file("tc-9999/agent_config_2", configs, "../test-certs/config-priv.pem")

# TC-0007 - kill_switch_conf

s = idd.iddd_pb2.Settings()
s.permanently_disable = True

configs = [("iddd", "Settings", s.SerializeToString())]

generate_file("tc-0007-data/kill_switch_conf", configs)

# TC-0002 - counters_expire_conf

iddd_settings = idd.iddd_pb2.Settings()
iddd_settings.expire_counters_age = 5

configs = [("iddd", "Settings", iddd_settings.SerializeToString()),
           ("iddd", "FilterConfig", exclude_filter.SerializeToString())]

generate_file("tc-0002-data/counters_expire_conf", configs)

# TC-0003 - sdata_expire_conf

iddd_settings = idd.iddd_pb2.Settings()
iddd_settings.expire_sdata_age = 5

configs = [("iddd", "Settings", iddd_settings.SerializeToString()),
           ("iddd", "FilterConfig", exclude_filter.SerializeToString())]

generate_file("tc-0003-data/sdata_expire_conf", configs)

# TC-0007 - remove_reports
ra = idd.iddd_pb2.RunActionsOnce()
ra.remove_reports = False

configs = [("iddd", "RunActionsOnce", ra.SerializeToString())]
generate_file("tc-0007-data/no_remove_reports_conf", configs)

# TC-0007 - remove_reports
ra = idd.iddd_pb2.RunActionsOnce()
ra.remove_reports  = True

configs = [("iddd", "RunActionsOnce", xd_ra.SerializeToString())]
generate_file("tc-0007-data/remove_reports_conf", configs)

# TC-0007 - study_one
iddd_settings = idd.iddd_pb2.Settings()
iddd_settings.study = "study_one"

configs = [("iddd", "Settings", iddd_settings.SerializeToString())]

generate_file("tc-0007-data/study_one_conf", configs)

# TC-0007 - study_two
iddd_settings = idd.iddd_pb2.Settings()
iddd_settings.study = "study_two"

configs = [("iddd", "Settings", iddd_settings.SerializeToString())]

# TC-0007 - study_two_nr2
iddd_settings = idd.iddd_pb2.Settings()
iddd_settings.study = "study_two"
iddd_settings.max_report_size = 100000

configs = [("iddd", "Settings", iddd_settings.SerializeToString())]

generate_file("tc-0007-data/study_two_nr2_conf", configs)

# TC-0007 - no_study
iddd_settings = idd.iddd_pb2.Settings()

configs = [("iddd", "Settings", iddd_settings.SerializeToString())]

generate_file("tc-0007-data/no_study_conf", configs)

# TC-0016 - IDD plugin

def addplugin(plugins, plugin_id, dir, report_prefix, report_prefix_packed, report_max_size, max_output_dir_size,
              plugin_type = idd.iddd_pb2.ConfigPlugin.Plugin.RCA,
              event_filter_type = idd.iddd_pb2.Filter.INCLUDE,
              counter_filter_type = idd.iddd_pb2.Filter.INCLUDE,
              staticdata_filter_type = idd.iddd_pb2.Filter.INCLUDE,
              report_header = True, encrypt = True, client_id = None, ignore_idd_enabled = None):
    plugin = plugins.plugin.add()
    plugin.plugin_id = plugin_id
    plugin.dir = dir
    plugin.report_prefix = report_prefix
    plugin.report_prefix_packed = report_prefix_packed
    plugin.report_max_size = report_max_size
    plugin.max_output_dir_size = max_output_dir_size
    plugin.type = plugin_type
    plugin.probe_filter.event_filter.type = event_filter_type
    plugin.probe_filter.counter_filter.type = counter_filter_type
    plugin.probe_filter.staticdata_filter.type = staticdata_filter_type
    plugin.report_header = report_header
    plugin.encrypt = encrypt
    if client_id != None:
        plugin.client_id = client_id
    if ignore_idd_enabled != None:
        plugin.ignore_idd_enabled = ignore_idd_enabled
    return plugin

# Plugin_type must be of type REGULAR since iddd expects absolute filepath (/rca/) for RCA.
cf = idd.iddd_pb2.ConfigPlugin()
plugin = addplugin(plugins = cf, plugin_id = idd.iddd_pb2.RCA_COMPACT,
                   plugin_type = idd.iddd_pb2.ConfigPlugin.Plugin.REGULAR, dir = "plugin1",
                   report_prefix = "p1", report_prefix_packed = "p1p", report_max_size = 1024,
                   max_output_dir_size = (2*1024), report_header = False, encrypt = False)
addfilter(plugin.probe_filter.event_filter, "p1", "probe1")
addfilter(plugin.probe_filter.event_filter, "p1", "probe2")
addfilter(plugin.probe_filter.event_filter, "p1", "probe3")
plugin = addplugin(plugins = cf, plugin_id = idd.iddd_pb2.RCA_COMPACT,
                  plugin_type = idd.iddd_pb2.ConfigPlugin.Plugin.REGULAR, dir = "plugin2",
                  report_prefix = "p2", report_prefix_packed = "p2p", report_max_size = 1024,
                  max_output_dir_size = (2*1024), report_header = False, encrypt = False)
addfilter(plugin.probe_filter.event_filter, "p2", "probe1")
plugin = addplugin(plugins = cf, plugin_id = idd.iddd_pb2.RCA_COMPACT,
                   plugin_type = idd.iddd_pb2.ConfigPlugin.Plugin.REGULAR, dir = "plugin3",
                   report_prefix = "p3", report_prefix_packed = "p3p", report_max_size = 1024,
                   max_output_dir_size = (2*1024), report_header = False)
addfilter(plugin.probe_filter.event_filter, "p3", "probe1")
addfilter(plugin.probe_filter.event_filter, "p3", "probe2")
addfilter(plugin.probe_filter.event_filter, "p3", "probe3")
addfilter(plugin.probe_filter.event_filter, "p3", "probe4")
addfilter(plugin.probe_filter.event_filter, "com.sonyericsson.idd.probe.android.devicemonitor.powerstat", "BootComplete")
plugin = addplugin(plugins = cf, plugin_id = idd.iddd_pb2.RCA_COMPACT,
                  plugin_type = idd.iddd_pb2.ConfigPlugin.Plugin.REGULAR, dir = "ignore_idd_enabled",
                  report_prefix = "iie",  report_prefix_packed = "iiep", report_max_size = 1024,
                  max_output_dir_size = (2*1024), report_header = False, ignore_idd_enabled = True)
addfilter(plugin.probe_filter.event_filter, "p1", "probe1")

filter = idd.iddd_pb2.FilterConfig()
filter.event_filter.type = idd.iddd_pb2.Filter.INCLUDE
addfilter(filter.event_filter, "idd", "probe1")
addfilter(filter.event_filter, "com.sonyericsson.idd.probe.android.devicemonitor.powerstat", "ShutDown")
addfilter(filter.event_filter, "idd.system_logging", "LogMessage")

configs = [("iddd", "FilterConfig", filter.SerializeToString()),
           ("iddd", "ConfigPlugin", cf.SerializeToString())]
generate_file("tc-0016-data/plugin1.config", configs)

del cf.plugin[1] # Remove plugin2
plugin = addplugin(plugins = cf, plugin_id = idd.iddd_pb2.XD,
                   plugin_type = idd.iddd_pb2.ConfigPlugin.Plugin.REGULAR, dir = "xd",
                   report_prefix = "xd", report_prefix_packed = "xdp", report_max_size = 1024,
                   max_output_dir_size = (2*1024))
addfilter(plugin.probe_filter.event_filter, "com.sonyericsson.idd.probe.android.devicemonitor.powerstat", "BootComplete")
addfilter(plugin.probe_filter.event_filter, "com.sonyericsson.idd.probe.android.devicemonitor.powerstat", "ShutDown")
addfilter(plugin.probe_filter.event_filter, "idd.system_logging", "LogMessage")

configs = [("iddd", "FilterConfig", filter.SerializeToString()),
           ("iddd", "ConfigPlugin", cf.SerializeToString()),
           ("idd", "LogReader", xd_rules)]
generate_file("tc-0016-data/plugin2.config", configs)

configs = [("iddd", "RunActions", xd_ra.SerializeToString()),
           ("iddd", "FilterConfig", filter.SerializeToString()),
           ("iddd", "ConfigPlugin", cf.SerializeToString()),
           ("idd", "LogReader", xd_rules),
           ("idd.system_logging", "ConfigLogreader", xd_cflr.SerializeToString())]
generate_file("tc-0016-data/plugin3.config", configs)

empty_include_filter = idd.iddd_pb2.FilterConfig()
empty_include_filter.event_filter.type = idd.iddd_pb2.Filter.INCLUDE
empty_include_filter.counter_filter.type = idd.iddd_pb2.Filter.INCLUDE
empty_include_filter.staticdata_filter.type = idd.iddd_pb2.Filter.INCLUDE

plugin = addplugin(plugins = cf, plugin_id = idd.iddd_pb2.RCA_NONCRASH, dir = "rca-noncrash",
                   report_prefix = "rcanonc", report_prefix_packed = "rcanoncp",
                   report_max_size = (200 * 1024), max_output_dir_size = int(10 * 1024 * 1024))
addfilter(plugin.probe_filter.event_filter, "com.sonyericsson.idd.probe.android.devicemonitor.fill_rca_partition", "*")
addfilter(plugin.probe_filter.event_filter, "iddtest", "AddEvent")

configs = [("iddd", "FilterConfig", empty_include_filter.SerializeToString()),
           ("iddd", "ConfigPlugin", cf.SerializeToString())]
generate_file("TC-0017-data/fill_rca_partition.config", configs)
