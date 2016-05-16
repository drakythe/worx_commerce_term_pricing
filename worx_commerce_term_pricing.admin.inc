<?php

function worx_commerce_term_pricing_settings_form() {
  $flavor = taxonomy_vocabulary_machine_name_load('product_flavor');
  $fl_terms = entity_load('taxonomy_term', FALSE, array('vid' => $flavor->vid));
  $strength = taxonomy_vocabulary_machine_name_load('product_strength');
  $st_terms = entity_load('taxonomy_term', FALSE, array('vid' => $strength->vid));
  $pgvg = taxonomy_vocabulary_machine_name_load('product_pg_vg');
  $pgvg_terms = entity_load('taxonomy_term', FALSE, array('vid' => $pgvg->vid));
  $flavor_options = $strength_options = $pgvg_options = $bulk_products = array();

  foreach ($fl_terms as $flt_obj) {
    $flavor_options[$flt_obj->tid] = $flt_obj->name;
  }
  foreach ($st_terms as $str_obj) {
    $strength_options[$str_obj->tid] = $str_obj->name;
  }
  foreach ($pgvg_terms as $pgvg_obj) {
    $pgvg_options[$pgvg_obj->tid] = $pgvg_obj->name;
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


  $form['contact_information'] = array(
    '#markup' => t("Add New Pricing Rule"),
    '#prefix' => '<strong>',
    '#suffix' => '</strong>',
  );
  $form['product'] = array(
    '#type' => 'select',
    '#title' => t('Product'),
    '#options' => $bulk_products,
    '#empty_option' => t('-Select Product-'),
    '#required' => TRUE,
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

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save')
  );

  return $form;
}

function worx_commerce_term_pricing_settings_form_submit($form, &$form_state) {
  dsm($form_state['values']);

  $values = $form_state['values'];
  $record = array();

  $record['product_sku'] = $values['product'];
  $record['flavor_tid'] = $values['flavor'];
  $record['strength_tid'] = $values['strength'];
  $record['pgvg_tid'] = $values['pgvg'];
  $record['price_change'] = $values['change'];
  $record['change_type'] = $values['change_type'];

  $write = drupal_write_record('commerce_term_pricing_rules', $record);

  if ($write === FALSE) {
    drupal_set_message('Oops, something went wrong', 'error');
  }

}
