<?php

/**
 * @TODO: The form should list the fields available on the line item type
 * attached to the node. Once selected and submitted, create option fields.
 * For text fields, a label and a required status
 * For term_lists, an option set to set which options are available and a
 *  required status
 * For image upload field, a max number of images dropdown.
 */

function worx_commerce_term_pricing_settings_form() {
  // @TODO: Need to dynamically create an array of variables
  $flavor_options = $strength_options = $pgvg_options = $bulk_products = array();
  $flavor = taxonomy_vocabulary_machine_name_load('product_flavor');
  $fl_terms = entity_load('taxonomy_term', FALSE, array('vid' => $flavor->vid));

  foreach ($fl_terms as $flt_obj) {
    $flavor_options[$flt_obj->tid] = $flt_obj->name;
  }
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
      l('Delete', 'admin/commerce/term_pricing/rule/' . $row->tprid . '/delete', array('query' => array('destination' => '/admin/commerce/term_pricing')))
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
    if (isset($option_set['options_data'])) {
      $saved_options[$option_set['nid']] = $option_set['options_data'];
    }
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
    array('data' => 'Edit', 'field' => 'edit'),
    array('data' => 'Delete', 'field' => 'tprid'),
  );

  foreach ($saved_options as $nid => $options) {
    $node = node_load($nid);
    $title = $node->title;
    $rows[] = array(
      $title,
      l("Edit Option Set Rule", 'admin/commerce/term_pricing/options/' . $nid),
      l("Delete Option Set Rule", 'admin/commerce/term_pricing/options/' . $nid . '/delete', array('query' => array('destination' => '/admin/commerce/term_pricing/options'))),
    );
  }
  foreach ($unsaved_options as $nid => $title) {
    $rows[] = array(
      $title,
      l("Add Option Set Rule", 'admin/commerce/term_pricing/options/' . $nid),
      "No option set created",
    );
  }

  $output = theme('table', array('header' => $header, 'rows' => $rows));
  $output .= theme('pager');

  return $output;
}

function worx_commerce_term_pricing_node_option_form() {
  $form = $possible_fields = $list_fields = $term_fields = $text_fields = $image_fields = array();
  $args = arg();
  $nid = $args[4];
  $node = node_load($nid);
  $node_title = $node->title;

  // @TODO: This needs to be a var set somewhere.
  $product_field = 'field_product_display_product';

  $info = field_info_instances('node', $node->type);
  $line_item_type = $info['field_product_display_product']['display']['default']['settings']['line_item_type'];
  $line_item_info = field_info_instances('commerce_line_item', $line_item_type);

  foreach ($line_item_info as $field => $properties) {
    // Setup filter arrays.
    $no_options = array(
      'commerce_display_path',
    );
    $optionable_types = array(
      'list_default',
      'taxonomy_term_reference_link',
      'text_default',
      'image',
    );

    // This will handle default fields which are the same type as fields we
    // want to create options for.
    if (in_array($field, $no_options)) {
      continue;
    }

    // Now we check to make sure we can handle the field type.
    if (in_array($properties['display']['default']['type'], $optionable_types)
    && $properties['commerce_cart_settings']['field_access']) {
      //dsm(field_info_field($field));
      $possible_fields[$field] = $properties['label'];

      switch ($properties['display']['default']['type']) {
        case 'list_default':
          $list_fields[$field] = $properties['label'];
          break;
        case 'taxonomy_term_reference_link':
          $term_fields[$field] = $properties['label'];
          break;
        case 'text_default':
          $text_fields[$field] = $properties['label'];
          break;
        case 'image':
          $image_fields[$field] = $properties['label'];
          break;
      }
    }
  }
  $defaults = db_select('commerce_term_pricing_node_options', 'ctpno')
    ->fields('ctpno')
    ->condition('nid', $nid)
    ->execute()
    ->fetchAssoc();
  $options = unserialize($defaults['options_data']);
  isset($options['available_fields']) ? $set_lists = $options['available_fields'] : $set_lists = array();

  // Now that we have all the pieces we actually create the form!
  // Need the nid for saving into the DB.
  $form['nid'] = array(
    '#type' => 'textfield',
    '#title' => t('Node'),
    '#default_value' => $nid,
    '#size' => 25,
    '#maxlength' => 100,
    '#required' => TRUE,
    '#access' => FALSE,
  );
  // Allow the user to select which attributes are available whne adding to cart
  $form['available_fields'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Available Attribute Fields'),
    '#description' => t('Select a field if it should be available as an attribute when adding a product to the cart.'),
    '#options' => $possible_fields,
    '#empty_option' => t('-None-'),
    '#default_value' => $set_lists,
  );

  // Each field type has specific fields, so we loop through the active ones
  // and create them.
  foreach ($list_fields as $list_field => $list_label) {
    if (array_key_exists($list_field, $set_lists)) {
      $form[$list_field] = array(
        '#type' => 'fieldset',
        '#title' => t($list_label),
        '#collapsible' => TRUE, // Added
        '#collapsed' => FALSE,  // Added
      );
      $form[$list_field][$list_field . '_required'] = array(
        '#type' => 'checkbox',
        '#title' => t($list_label . ' Field Required?.'),
      );
      $form[$list_field][$list_field . '_label'] = array(
        '#type' => 'textfield',
        '#title' => t($list_label . ' field Label'),
        '#default_value' => $list_label,
        '#description' => t('Change the field label if desired.'),
        '#size' => 60,
        '#maxlength' => 128,
        '#required' => TRUE,
      );
      //Get list options and allow users to select availability
      $list_options = field_info_field($list_field)['settings']['allowed_values'];
      $form[$list_field][$list_field . '_options'] = array(
        '#type' => 'checkboxes',
        '#title' => t($list_label . ' Options'),
        '#options' => $list_options,
        '#default_value' => array(), //@TODO Set default value properly.
        '#description' => t('Select which options are available for this attribute on this product.'),
      );
    }
  }

  foreach ($term_fields as $term_field => $term_label) {
    if (array_key_exists($term_field, $set_lists)) {
      $form[$term_field] = array(
        '#type' => 'fieldset',
        '#title' => t($term_label),
        '#collapsible' => TRUE, // Added
        '#collapsed' => FALSE,  // Added
      );
      $form[$term_field][$term_field . '_required'] = array(
        '#type' => 'checkbox',
        '#title' => t($term_label . ' Field Required?.'),
      );
      $form[$term_field][$term_field . '_label'] = array(
        '#type' => 'textfield',
        '#title' => t($term_label . ' field Label'),
        '#default_value' => $term_label,
        '#description' => t('Change the field label if desired.'),
        '#size' => 60,
        '#maxlength' => 128,
        '#required' => TRUE,
      );
    }
    $term_options = array();
    $term_vocab = taxonomy_vocabulary_machine_name_load(field_info_field($term_field)['settings']['allowed_values'][0]['vocabulary']);
    $vocab_tree = taxonomy_get_tree($term_vocab->vid);
    foreach ($vocab_tree as $term) {
      $term_options[$term->tid] = $term->name;
    }
    $form[$term_field][$term_field . '_options'] = array(
      '#type' => 'checkboxes',
      '#title' => t($term_label . ' Options'),
      '#options' => $term_options,
      '#default_value' => array(), //@TODO Set default value properly.
      '#description' => t('Select which options are available for this attribute on this product.'),
    );
  }
  
  foreach ($text_fields as $text_field => $text_label) {
    if (array_key_exists($text_field, $set_lists)) {
      $form[$text_field] = array(
        '#type' => 'fieldset',
        '#title' => t($text_label),
        '#collapsible' => TRUE, // Added
        '#collapsed' => FALSE,  // Added
      );
      $form[$text_field][$text_field . '_required'] = array(
        '#type' => 'checkbox',
        '#title' => t($text_label . ' Field Required?.'),
      );
      $form[$text_field][$text_field . '_label'] = array(
        '#type' => 'textfield',
        '#title' => t($text_label . ' field Label'),
        '#default_value' => $text_label,
        '#description' => t('Change the field label if desired.'),
        '#size' => 60,
        '#maxlength' => 128,
        '#required' => TRUE,
      );
    }
  }
  
  foreach ($image_fields as $image_field => $image_label) {
    if (array_key_exists($image_field, $set_lists)) {
      $form[$image_field] = array(
        '#type' => 'fieldset',
        '#title' => t($image_label),
        '#collapsible' => TRUE, // Added
        '#collapsed' => FALSE,  // Added
      );
      $form[$image_field][$image_field . '_required'] = array(
        '#type' => 'checkbox',
        '#title' => t($image_label . ' Field Required?.'),
      );
      $form[$image_field][$image_field . '_label'] = array(
        '#type' => 'textfield',
        '#title' => t($image_label . ' field Label'),
        '#default_value' => $image_label,
        '#description' => t('Change the field label if desired.'),
        '#size' => 60,
        '#maxlength' => 128,
        '#required' => TRUE,
      );
      $form[$image_field][$image_field . '_max'] = array(
        '#type' => 'select',
        '#title' => t('Maximum Number of Images'),
        '#options' => range(0, 1),
        '#empty_option' => t('-none-'),
        '#required' => FALSE,
        '#default_value' => array(), //@TODO Properly set this default value.
      );
    }
  }

  $form['#validate'][] = 'worx_commerce_term_pricing_options_validate';
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save')
  );

  return $form;
}

function worx_commerce_term_pricing_options_validate($form, $form_state) {
  $lists = array_filter($form_state['values']['available_fields']);

  //@TODO: This hardcoded bit needs to be dynamic
  /**
  $flavors = array_filter($form_state['values']['flavor']);
  $strength = array_filter($form_state['values']['strength']);
  $pgvg = array_filter($form_state['values']['pgvg']);
  if (isset($lists['product_flavor']) && empty($flavors)) {
    form_set_error('flavor', 'Flavor cannot be be enabled as an attribute without any options enabled.');
  }
  if (isset($lists['product_strength']) && empty($strength)) {
    form_set_error('strength', 'Strength cannot be be enabled as an attribute without any options enabled.');
  }
  if (isset($lists['product_pg_vg']) && empty($pgvg)) {
    form_set_error('pgvg', 'PG/VG cannot be be enabled as an attribute without any options enabled.');
  }
   */
}

function worx_commerce_term_pricing_node_option_form_submit($form, &$form_state) {
  $values = $form_state['values'];
  $nid = $values['nid'];
  $lists = array_filter($values['available_fields']);
  unset($values['submit'], $values['form_build_id'], $values['form_token'],
    $values['form_id'], $values['op'], $values['nid'],
    $values['available_fields']);

  $exists = db_select('commerce_term_pricing_node_options', 'ctpno')
    ->fields('ctpno')
    ->condition('nid', $nid)
    ->execute()
    ->fetchAssoc();

  $record = array(
    'nid' => $nid,
    'options_data' => array(
      'available_fields' => $lists,
    ),
  );
  
  foreach ($values as $field_name => $field_option_values) {
    foreach ($field_option_values as $option => $values) {
      $record['options_data'][$option] = $values;
    }
  }

  if (!empty($exists)) {
    drupal_write_record('commerce_term_pricing_node_options', $record, 'nid');
  }
  else {
    drupal_write_record('commerce_term_pricing_node_options', $record);
  }
}
