<?php

function worx_commerce_term_pricing_settings_form() {
  $flavor_options = $strength_options = $pgvg_options = $bulk_products = array();
  $flavor = taxonomy_vocabulary_machine_name_load('product_flavor');
  $fl_terms = entity_load('taxonomy_term', FALSE, array('vid' => $flavor->vid));

  foreach ($fl_terms as $flt_obj) {
    $flavor_options[$flt_obj->tid] = $flt_obj->name;
  }
  // @TODO: We won't be using these rules to begin with, but may enable them
  // later.
  /**
  $strength = taxonomy_vocabulary_machine_name_load('product_strength');
  $st_terms = entity_load('taxonomy_term', FALSE, array('vid' => $strength->vid));
  $pgvg = taxonomy_vocabulary_machine_name_load('product_pg_vg');
  $pgvg_terms = entity_load('taxonomy_term', FALSE, array('vid' => $pgvg->vid));

  foreach ($st_terms as $str_obj) {
    $strength_options[$str_obj->tid] = $str_obj->name;
  }
  foreach ($pgvg_terms as $pgvg_obj) {
    $pgvg_options[$pgvg_obj->tid] = $pgvg_obj->name;
  }
  */

  $query = new EntityFieldQuery();
  $query->entityCondition('entity_type', 'commerce_product')
    ->entityCondition('bundle', 'term_priced')
    ->propertyCondition('status', 1)
    ->fieldOrderBy('commerce_price', 'amount', 'ASC')
  ;
  $result = $query->execute();
  foreach ($result['commerce_product'] as $product_obj) {
    $product = commerce_product_load($product_obj->product_id);
    $bulk_products[$product->sku] = $product->title;
  }

  $rules_table = worx_commerce_term_pricing_get_rules();

  $form['rules_table'] = array(
    '#markup' => $rules_table,
  );
  $form['contact_information'] = array(
    '#markup' => t("Add New Pricing Rule"),
    '#prefix' => '<strong>',
    '#suffix' => '</strong>',
  );
  $form['product'] = array(
    '#type' => 'select',
    '#title' => t('Product'),
    '#options' => $bulk_products,
    '#empty_option' => t('-none-'),
    '#required' => FALSE,
    '#default_value' => '',
    '#prefix' => '<table><tr><td>',
    '#suffix' => '</td>',
   );
  $form['flavor'] = array(
    '#type' => 'select',
    '#title' => t('Flavor'),
    '#options' => $flavor_options,
    '#empty_option' => t('-None-'),
    '#default_value' => '',
    '#prefix' => '<td>',
    '#suffix' => '</td>',
   );
  // @TODO: We won't be using these rules to begin with, but may enable them
  // later.
  /**
  $form['strength'] = array(
    '#type' => 'select',
    '#title' => t('Strength'),
    '#options' => $strength_options,
    '#empty_option' => t('-None-'),
    '#default_value' => '',
    '#prefix' => '<td>',
    '#suffix' => '</td>',
   );
  $form['pgvg'] = array(
    '#type' => 'select',
    '#title' => t('PG/VG Ratio'),
    '#options' => $pgvg_options,
    '#empty_option' => t('-None-'),
    '#default_value' => '',
    '#prefix' => '<td>',
    '#suffix' => '</td>',
   );
   */
  $form['change'] = array(
    '#type' => 'textfield',
    '#title' => t('Price Change'),
    '#default_value' => '',
    '#size' => 25,
    '#maxlength' => 100,
    '#required' => TRUE,
    '#prefix' => '<td>',
    '#suffix' => '</td>',
  );
  $form['change_type'] = array(
    '#type' => 'select',
    '#title' => t('Price Change Type'),
    '#options' => array(
      'flat' => 'Flat Increase/decrease',
      'percent' => 'Percent Change',
    ),
    '#default_value' => array('flat'),
    '#prefix' => '<td>',
    '#suffix' => '</td></tr></table>',
  );

  $form['#validate'][] = 'worx_commerce_term_pricing_rule_validate';

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save')
  );

  return $form;
}

function worx_commerce_term_pricing_rule_validate($form, &$form_state) {
  $values = $form_state['values'];

  if (!is_numeric($values['change'])) {
    form_set_error('change', 'Please set a valid amount for price change');
  }

  $valid = FALSE;

  if (!empty($values['flavor'])) {
    $valid = TRUE;
  }
  // @TODO: We won't be using these rules to begin with, but may enable them
  // later.
  /**
  else if (!empty($values['strength'])) {
    $valid = TRUE;
  }
  else if (!empty($values['pgvg'])) {
    $valid = TRUE;
  }
   */

  if (!$valid) {
    form_set_error('', 'Please select an attribute to apply this pricing rule to.');
  }
}

function worx_commerce_term_pricing_settings_form_submit($form, &$form_state) {
  $values = $form_state['values'];
  $record = array();

  // @TODO: We won't be using these rules to begin with, but may enable them
  // later.
  //$record['strength_tid'] = $values['strength'];
  //$record['pgvg_tid'] = $values['pgvg'];

  $record['product_sku'] = $values['product'];
  $record['flavor_tid'] = $values['flavor'];
  $record['price_change'] = $values['change'];
  $record['change_type'] = $values['change_type'];

  $write = drupal_write_record('commerce_term_pricing_rules', $record);

  if ($write === FALSE) {
    drupal_set_message('Rule failed to write to Database, inform site Administrator', 'error');
    watchdog('Term Pricing', '<pre>' . print_r($record, TRUE) . '</pre>', array(), WATCHDOG_ERROR);
  }
}

function worx_commerce_term_pricing_get_rules() {
  $header = array(
    array('data' => 'Product', 'field' => 'product_sku', 'sort' => 'ASC'),
    array('data' => 'Flavor', 'field' => 'flavor_tid', 'sort' => 'ASC'),
    array('data' => 'Change', 'field' => 'price_change', 'sort' => 'ASC'),
    array('data' => 'Change Type', 'field' => 'change_type'),
    array('data' => 'Delete', 'field' => 'tprid'),
  );

  $select = db_select('commerce_term_pricing_rules', 'ctpr')
    ->extend('PagerDefault')
    ->extend('TableSort');

  $select->fields('ctpr', array('tprid', 'product_sku', 'flavor_tid', 'price_change', 'change_type'))
    ->limit(10)
    ->orderByHeader($header);

  $results = $select->execute();

  $rows = array();
  foreach ($results as $row) {
    if (!empty($row->product_sku)) {
      $product = commerce_product_load_by_sku($row->product_sku);
      $title = $product->title;
      $title = $title . '(' . $row->product_sku . ')';
    }
    else {
      $title = 'N/A';
    }
    $flavor_term = taxonomy_term_load($row->flavor_tid);
    $flavor_name = $flavor_term->name;

    $rows[] = array(
      $title,
      $flavor_name,
      $row->price_change,
      $row->change_type,
      '<a href="/admin/commerce/term_pricing/rule/' . $row->tprid . '/delete?destination=/admin/commerce/term_pricing">Delete</a>',
    );
  }

  $output = theme('table', array('header' => $header, 'rows' => $rows));
  $output .= theme('pager');

  return $output;
}

function worx_commerce_term_pricing_options_form() {
  $form = $node_options = $saved_options = array();
  $nodes = node_load_multiple(array(), array('type' => 'bulk_product_display'));
  foreach ($nodes as $node) {
    $node_options[$node->nid] = $node->title;
  }
  $saved_nodes = db_select('commerce_term_pricing_node_options', 'ctpno')
    ->fields('ctpno')
    ->execute();
  while ($option_set = $saved_nodes->fetchAssoc()) {
    $saved_options[$option_set['nid']] = $saved_options['options_data'];
  }

  $unsaved_options = array_diff_key($node_options, $saved_options);

  $table = worx_commerce_term_pricing_options_create_table($unsaved_options, $saved_options);

  $form['markup'] = array(
    '#markup' => $table,
  );

  return $form;
}

function worx_commerce_term_pricing_options_create_table($unsaved_options, $saved_options) {
  $rows = array();
  $header = array(
    array('data' => 'Product', 'field' => 'product', 'sort' => 'ASC'),
    array('data' => 'Flavor', 'field' => 'flavor', 'sort' => 'ASC'),
    array('data' => 'Delete', 'field' => 'tprid'),
  );

  foreach ($saved_options as $nid => $options) {
    $node = node_load($nid);
    $title = $node->title;
    $rows[] = array(
      $title,
      '<a href="/admin/commerce/term_pricing/options/' . $nid . '">Edit Option Set Rule</a>',
      '<a href="/admin/commerce/term_pricing/options/' . $nid . '/delete">Delete Option Set Rule</a>',
    );
  }
  foreach ($unsaved_options as $nid => $title) {
    $rows[] = array(
      $title,
      '<a href="/admin/commerce/term_pricing/options/' . $nid . '">Add Option Set Rule</a>',
      '<a href="/admin/commerce/term_pricing/options/' . $nid . '/delete">Delete Option Set Rule</a>',
    );
  }

  $output = theme('table', array('header' => $header, 'rows' => $rows));
  $output .= theme('pager');

  return $output;
}

function worx_commerce_term_pricing_node_option_form() {
  $form = array();
  $args = arg();
  $nid = $args[4];
  $node = node_load($nid);
  $node_title = $node->title;

  $flavor = taxonomy_vocabulary_machine_name_load('product_flavor');
  $fl_terms = entity_load('taxonomy_term', FALSE, array('vid' => $flavor->vid));
  $strength = taxonomy_vocabulary_machine_name_load('product_strength');
  $st_terms = entity_load('taxonomy_term', FALSE, array('vid' => $strength->vid));
  $pgvg = taxonomy_vocabulary_machine_name_load('product_pg_vg');
  $pgvg_terms = entity_load('taxonomy_term', FALSE, array('vid' => $pgvg->vid));
  $flavor_options = $strength_options = $pgvg_options = array();

  foreach ($fl_terms as $flt_obj) {
    $flavor_options[$flt_obj->tid] = $flt_obj->name;
  }
  foreach ($st_terms as $str_obj) {
    $strength_options[$str_obj->tid] = $str_obj->name;
  }
  foreach ($pgvg_terms as $pgvg_obj) {
    $pgvg_options[$pgvg_obj->tid] = $pgvg_obj->name;
  }

  $form['nid'] = array(
    '#type' => 'textfield',
    '#title' => t('Node'),
    '#default_value' => $nid,
    '#size' => 25,
    '#maxlength' => 100,
    '#required' => TRUE,
    '#access' => FALSE,
  );
  $form['my_node'] = array(
    '#markup' => "Set option lists for $node_title",
  );
  $form['flavor'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Flavor'),
    '#options' => $flavor_options,
    '#empty_option' => t('-None-'),
    '#default_value' => array(),
    '#prefix' => '<table><tr><td>',
    '#suffix' => '</td>',
  );
  $form['strength'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Strength'),
    '#options' => $strength_options,
    '#empty_option' => t('-None-'),
    '#default_value' => array(),
    '#prefix' => '<td>',
    '#suffix' => '</td>',
  );
  $form['pgvg'] = array(
    '#type' => 'checkboxes',
    '#title' => t('PG/VG Ratio'),
    '#options' => $pgvg_options,
    '#empty_option' => t('-None-'),
    '#default_value' => array(),
    '#prefix' => '<td>',
    '#suffix' => '</td></tr></table>',
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save')
  );

  return $form;
}

function worx_commerce_term_pricing_node_option_form_submit($form, &$form_state) {
  $nid = $form_state['values']['nid'];
  $flavors = array_filter($form_state['values']['flavor']);
  $strength = array_filter($form_state['values']['strength']);
  $pgvg = array_filter($form_state['values']['pgvg']);
  
  $record = array(
    'nid' => $nid,
    'options_data' => array(
      'flavors' => $flavors,
      'strength' => $strength,
      'pgvg' => $pgvg,
    ),
  );
  
  drupal_write_record('commerce_term_pricing_node_options', $record);
}
