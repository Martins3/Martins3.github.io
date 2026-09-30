// SPDX-License-Identifier: GPL-3.0

#[test]
fn test_parse_static_network() {
    assert_eq!(
        super::parse_static_network("52:54:00:00:60:00,10.0.96.0/16"),
        Some(("52:54:00:00:60:00", "10.0.96.0/16"))
    );
    assert_eq!(super::parse_static_network("missing-address"), None);
    assert_eq!(super::parse_static_network(",10.0.96.0/16"), None);
}

#[test]
fn test_network_configuration_matches_mac_role() {
    let static_network = Some(("52:54:00:00:60:00", "10.0.96.0/16"));
    let dhcp_mac = Some("52:54:00:12:34:56");

    assert_eq!(
        super::network_configuration("52:54:00:00:60:00", static_network, dhcp_mac),
        Some(super::NetworkConfiguration::Static(
            "10.0.96.0/16".to_string()
        ))
    );
    assert_eq!(
        super::network_configuration("52:54:00:12:34:56", static_network, dhcp_mac),
        Some(super::NetworkConfiguration::Dhcp)
    );
    assert_eq!(
        super::network_configuration("52:54:00:de:ad:be", static_network, dhcp_mac),
        None
    );
}
